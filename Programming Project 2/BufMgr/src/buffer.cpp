/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb
{

BufMgr::BufMgr(std::uint32_t bufs)
	: numBufs(bufs)
{
	bufDescTable = new BufDesc[bufs];

	for (FrameId i = 0; i < bufs; i++)
	{
		bufDescTable[i].frameNo = i;
		bufDescTable[i].valid = false;
	}

	bufPool = new Page[bufs];

	int htsize = ((((int)(bufs * 1.2)) * 2) / 2) + 1;

	// allocate the buffer hash table
	hashTable = new BufHashTbl(htsize); 

	clockHand = bufs - 1;
}

BufMgr::~BufMgr()
{
	// loop over all page frames
	for (uint32_t i = 0; i < numBufs; i++)
	{
		// first check if the file is already NULL
		if (bufDescTable[i].file != NULL)
		{
			if (bufDescTable[i].dirty)
			{
				// write page back if dirty, and set dirty to false
				bufDescTable[i].file->writePage(bufPool[bufDescTable[i].frameNo]);
				bufDescTable[i].dirty = false;
			}

			// remove from hash table
			hashTable->remove(bufDescTable[i].file, bufDescTable[i].pageNo);

			// clear the buffer description table
			bufDescTable[i].Clear();
		}
	}

	// delete buffer description table and buffer pool
	delete[] bufDescTable;
	delete[] bufPool;
}

void BufMgr::advanceClock()
{
	//Advance clock to next frame in the buffer pool
	clockHand = (clockHand + 1) % numBufs;
}

void BufMgr::allocBuf(FrameId &frame)
{
	// need to loop over the buffer pool 2 times in the worst case
	int count = 2;
	while (count > 0) 
	{
		// loop over all page frames
		for (uint32_t i = 0; i < numBufs; i++)
		{
			advanceClock();

			// current frame
			BufDesc *curr = &bufDescTable[clockHand];

			if (curr->valid == true)
			{
				if (curr->refbit == true)
				{
					// set refbit to false if it was true
					curr->refbit = false;
				}
				else
				{
					if (curr->pinCnt == 0)
					{
						if (curr->dirty == true)
						{
							// write back current page if it is dirty
							curr->file->writePage(bufPool[bufDescTable[clockHand].frameNo]);
						}

						// remove from hash table
						hashTable->remove(curr->file, bufDescTable[clockHand].pageNo);

						// clear the buffer description table
						bufDescTable[clockHand].Clear();

						// return the current clockhand
						frame = clockHand;
						return;
					}
				}
			}
			else
			{
				frame = clockHand;
				return;
			}
		}
		count = count - 1;
	}
	throw BufferExceededException();
}

void BufMgr::readPage(File *file, const PageId pageNo, Page *&page)
{
	FrameId frameNo;
	try
	{
		//Check if page is in bufferpool
		hashTable->lookup(file, pageNo, frameNo);

		//No Exception
		//Set Reference Bit
		bufDescTable[frameNo].refbit = true;

		//Increment Pin Count
		bufDescTable[frameNo].pinCnt = bufDescTable[frameNo].pinCnt + 1;
	}
	catch (HashNotFoundException e1)
	{

		//Page not in bufferPool, so allocate a buffer frame
		allocBuf(frameNo);

		//Read page from disk
		Page newPage = file->readPage(pageNo);

		//Place the read page in the bufferPool frame
		bufPool[frameNo] = newPage;

		//Add an entry to hash table
		hashTable->insert(file, pageNo, frameNo);

		//Set member values of the corresponding frame
		bufDescTable[frameNo].Set(file, pageNo);
	}

	//Finally for try-catch
	//Pointer to the frame containing the page
	page = &(bufPool[frameNo]);
}

void BufMgr::unPinPage(File *file, const PageId pageNo, const bool dirty)
{
	try
	{
		// create a new frame id
		FrameId i; 

		// look up the frame id given the file and page number
		hashTable->lookup(file, pageNo, i);

		if (bufDescTable[i].pinCnt == 0)
		{
			// throw an error if pin count is 0
			throw PageNotPinnedException(file->filename(), pageNo, i);
		}
		else
		{
			if (dirty == true)
			{
				// set bufDescTable[i].dirty to true if dirty is true
				bufDescTable[i].dirty = true;
			}

			// decrement pin count by 1
			bufDescTable[i].pinCnt--;
		}
	}
	catch (HashNotFoundException e)
	{
	}
}

void BufMgr::allocPage(File *file, PageId &pageNo, Page *&page)
{
	//Allocate an empty page in the specified file
	Page newPage = file->allocatePage();

	//Get the page number
	pageNo = newPage.page_number();

	//Get a buffer pool frame
	FrameId frameNo;
	allocBuf(frameNo);

	//Place the page in the bufferpool frame
	bufPool[frameNo] = newPage;

	//Add an entry to hash table
	hashTable->insert(file, pageNo, frameNo);

	//Set member values of the corresponding frame
	bufDescTable[frameNo].Set(file, pageNo);

	//Pointer to the frame containing the page
	page = &(bufPool[frameNo]);
}

void BufMgr::disposePage(File *file, const PageId PageNo)
{
	//New a frame number
	FrameId frame_Number;

	try
	{
		// Check the hashtable to see whether the page to be deleted is allocated a frame in the buffer pool
		hashTable->lookup(file, PageNo, frame_Number);

		//Free the corresponding frame
		bufDescTable[frame_Number].Clear();

		//Remove the correspondingly entry from hash table
		hashTable->remove(file, PageNo);
	}

	catch (HashNotFoundException e1)
	{

	}

	//Delete the particular page from file
	file->deletePage(PageNo);
}

void BufMgr::flushFile(const File *file)
{
	//Scan bufTable
	for (uint32_t i = 0; i < numBufs; i++)
	{
		//Check whether the encountered page belonging to the file
		if (bufDescTable[i].file == file)
		{
			//Throws PagePinnedException if the encountered page of the file is pinned
			if (bufDescTable[i].pinCnt > 0)
			{
				throw PagePinnedException(file->filename(), bufDescTable[i].pageNo, bufDescTable[i].frameNo);
			}
			//Throws BadBuffer-Exception if an invalid page belonging to the file is encountered
			if (!bufDescTable[i].valid)
			{
				throw BadBufferException(bufDescTable[i].frameNo, bufDescTable[i].dirty, bufDescTable[i].valid, bufDescTable[i].refbit);
			}
			//Check whether the page is dirty
			if (bufDescTable[i].dirty)
			{
				//Call file->writePage() to flush the page to disk
				bufDescTable[i].file->writePage(bufPool[bufDescTable[i].frameNo]);

				//Set the dirty bit for the page to false
				bufDescTable[i].dirty = false;
			}
			//Remove the page from the hashtable (whether the page is clean or dirty)
			hashTable->remove(bufDescTable[i].file, bufDescTable[i].pageNo);

			//Invoke the Clear() method of BufDesc for the page frame
			bufDescTable[i].Clear();
		}
	}
}

void BufMgr::printSelf(void)
{
	BufDesc *tmpbuf;
	int validFrames = 0;

	for (std::uint32_t i = 0; i < numBufs; i++)
	{
		tmpbuf = &(bufDescTable[i]);
		tmpbuf->Print();

		if (tmpbuf->valid == true)
			validFrames++;
	}
}

} // namespace badgerdb
