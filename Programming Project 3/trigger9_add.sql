-- description: Constraint 13

PRAGMA foreign_keys = ON;

drop trigger if exists trigger9;

create trigger trigger9
	after insert on Bids
	for each row when (NEW.Amount >= (Select i.Buy_Price from Items i where NEW.ItemID = i.ItemID))
	begin
		UPDATE Items SET Ends = NEW.Time WHERE New.ItemID = Items.ItemID;
	end;