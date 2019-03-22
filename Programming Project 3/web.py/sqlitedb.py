import web

db = web.database(dbn='sqlite',
        db='AuctionBase' #TODO: add your SQLite database filename
    )

######################BEGIN HELPER METHODS######################

# Enforce foreign key constraints
# WARNING: DO NOT REMOVE THIS!
def enforceForeignKey():
    db.query('PRAGMA foreign_keys = ON')

# initiates a transaction on the database
def transaction():
    return db.transaction()
# Sample usage (in auctionbase.py):
#
# t = sqlitedb.transaction()
# try:
#     sqlitedb.query('[FIRST QUERY STATEMENT]')
#     sqlitedb.query('[SECOND QUERY STATEMENT]')
# except Exception as e:
#     t.rollback()
#     print str(e)
# else:
#     t.commit()
#
# check out http://webpy.org/cookbook/transactions for examples

# returns the current time from your database
def getTime():
    # TODO: update the query string to match
    # the correct column and table name in your database
    query_string = 'select Time from CurrentTime'
    results = query(query_string)
    # alternatively: return results[0]['currenttime']
    return results[0].Time # TODO: update this as well to match the
                                  # column name


def setTime(time):
    # TODO: update the query string to match
    # the correct column and table name in your database
    query_string = 'Update CurrentTime set Time = $currTime'
    
    try:
        db.query(query_string, {'currTime': time})
        return True
    except Exception as e:
        return False

    

# returns a single item specified by the Item's ID in the database
# Note: if the `result' list is empty (i.e. there are no items for a
# a given ID), this will throw an Exception!
def getItemById(item_id):
    # TODO: rewrite this method to catch the Exception in case `result' is empty
    query_string = 'select * from Items where item_ID = $itemID'
    result = query(query_string, {'itemID': item_id})
    return result[0]

# wrapper method around web.py's db.query method
# check out http://webpy.org/cookbook/query for more info
def query(query_string, vars = {}):
    return list(db.query(query_string, vars))

#####################END HELPER METHODS#####################

#TODO: additional methods to interact with your database,
# e.g. to update the current time
def addBid(post_params):
    ItemID = post_params['itemID']
    UserID = post_params['userID']
    Price = post_params['price']
    try:
        query_string ="Select Time from CurrentTime"
        time = query(query_string)[0].Time
        query_string = "Insert INTO Bids(ItemID,UserID,Amount,Time) VALUES ($ItemID, $UserID, $Price, $time)"
        result = db.query(query_string, {"ItemID":ItemID,"UserID":UserID,"Price":Price, "time": time})
        # # update Items table
        # try:
        #     query_string ="Select Number_of_Bids from Items where ItemID = $ItemID"
        #     NumBids = query(query_string, {"ItemID":ItemID})[0].Number_of_Bids
        #     query_string ="Update Items set Currently = $Price, Number_of_Bids = $Number_of_Bids, where ItemID = $ItemID"
        #     db.query(query_string, {"Price":Price,"Number_of_Bids":NumBids+1})
        # except Exception as e:
        #     print e 

        
        return True
        
                
    except Exception as e:
        return False 

def search(post_params):
    itemID = post_params['itemID']
    category = post_params['category']
    description = post_params['description']
    minPrice = post_params['minPrice']
    maxPrice = post_params['maxPrice']
    status = post_params['status'];

    query_string = 'select * from Items, Categories, CurrentTime where Items.ItemID = Categories.ItemID'

    if itemID:
        query_string += ' and Items.ItemID = $itemID'

    if category:
        query_string += ' and Categories.Category = $category'

    if description:
        query_string += ' and Items.Description like $description'

    if minPrice:
        query_string += ' and Items.Currently >= $minPrice'

    if maxPrice:
        query_string += ' and Items.Currently <= $maxPrice'


    if status == 'open':
        query_string += ' and CurrentTime.Time >= Items.Started and CurrentTime.Time < Items.Ends'
    elif status == 'close':
        query_string += ' and CurrentTime.Time >= Items.Ends'
    elif status == 'notStarted':
        query_string += ' and CurrentTime.Time < Items.Started'
    

    results = query(query_string, {'itemID':itemID, 'category':category, 'description':"%"+description+"%", 'minPrice':minPrice, 'maxPrice':maxPrice})
    
    return results

def getDetails(itemID):
    query_string1 = 'select * from Items where ItemID = $itemID'

    ItemDetails = query(query_string1, {'itemID':itemID})

    query_string2 = 'select Category from Categories where ItemID = $itemID'

    Categories = query(query_string2, {'itemID':itemID})

    
    result = {}

    result['Item Details'] = {}

    result['Item Details']['ItemsID'] = ItemDetails[0]['ItemID']
    result['Item Details']['Title'] = ItemDetails[0]['Name']
    result['Item Details']['Seller'] = ItemDetails[0]['Seller_UserID']
    result['Item Details']['Description'] = ItemDetails[0]['Description']
    result['Categories List'] = []
    result['Bids on the Item'] = []

    for category in Categories:
        result['Categories List'].append(category['Category'])

    currentTime = getTime()

    winnerExists = False
    
    if(currentTime >= ItemDetails[0]['Started'] and currentTime < ItemDetails[0]['Ends']):
        result['Item Details']['Auction Status'] = "OPEN"
    elif(currentTime >= ItemDetails[0]['Ends']):
        result['Item Details']['Auction Status'] = "CLOSED"
        winnerExists = True
    else:
        result['Item Details']['Auction Status'] = "NOT YET STARTED"


    query_string3 = 'select UserID,Amount,Time from Bids where ItemID = $itemID'

    Bids = query(query_string3, {'itemID':itemID})

    maxBid = 0
    winner = None

    for bid in Bids:
        if(bid['Amount']>maxBid):
            winner = bid['UserID']
            maxBid = bid['Amount']

        result['Bids on the Item'].append({'user':bid['UserID'],'amount':'$'+str(bid['Amount']),'time':bid['Time']})


    if(winnerExists):
        result['Item Details']['Winner of Auction'] = winner
    

    return result