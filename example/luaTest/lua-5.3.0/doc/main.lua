
local Proto = require("proto4z")
print("ssssssss")
Proto:test()
local test = {
				_id = {val = 1000, type="i32"}, 
				_user = {val = "user", type="string"},
				_items = {val = {
									_itemID = {val = 10, type="ui16"},
			  						[1] = "_itemID" 
			  					},
			  			 type = "table"
			  			 },
				[1] = "_id",
				[2] = "_user",
				[3] = "_items",
			 }

print(#test)


local data = Proto:pack(test, "table")
local hex = ""
for i = 1, #data do
	hex = hex .. string.format("%02x ",string.byte(data, i))
end
print(hex)

