select count(distinct B.UserID) from Item as I,Bid as B where I.SellerID=B.UserID;
