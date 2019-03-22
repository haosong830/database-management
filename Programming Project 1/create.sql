drop table if exists User;
drop table if exists Item;
drop table if exists Bid;
drop table if exists Belongs;
drop table if exists Category;

create table User (UserID TEXT PRIMARY KEY,Rating INTEGER,Location TEXT,Country TEXT);


create table Item (ItemID INTEGER PRIMARY KEY,Name TEXT,Currently REAL,Buy_Price TEXT,First_Bid REAL,Number_of_Bids INTEGER,Started DATETIME,Ends DATETIME,SellerID TEXT,Description TEXT,FOREIGN KEY (SellerID) REFERENCES User(UserID));


create table Bid (UserID TEXT,ItemID INTEGER,Amount REAL,Time DATETIME, PRIMARY KEY(UserID,ItemID,Amount,Time),FOREIGN KEY (ItemID) REFERENCES Item(ItemID),FOREIGN KEY (UserID) REFERENCES User(UserID));

create table Belongs (ItemID INTEGER,CategoryName TEXT, PRIMARY KEY(ItemID,CategoryName), FOREIGN KEY (ItemID) REFERENCES Item(ItemID), FOREIGN KEY (CategoryName) REFERENCES Category(CategoryName));

create table Category (CategoryName TEXT, PRIMARY KEY(CategoryName));
