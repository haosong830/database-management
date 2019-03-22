select count(distinct CategoryName) from Belongs as C,Bid as B where C.ItemID=B.ItemID and B.Amount>100; 
