select ItemID from Item where Currently=(select max(Currently) from Item);
