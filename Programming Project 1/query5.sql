select count(distinct U.UserID) from User as U,Item as I where U.UserID=I.SellerID and U.Rating>1000;
