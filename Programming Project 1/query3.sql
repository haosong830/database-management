select count(ItemID) from Item where ItemID in (select ItemID from Belongs group by ItemID having count(CategoryName)=4);


