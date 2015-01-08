
local ProtoCore = require("proto4z")

local LoginReq = 
{
	_id = {val = 1000, type="i32"}, 
	_user = {val = "user", type="string"},
	_info = {val = 	{
						_infoID = {val = 10, type="ui16"},
						[1] = "_infoID" 
					},
			 type = "struct",
			 },
	_items = {val = {
						10,
						20,
						30,
						40
					},
			 type = "array",
			 vtype="i16",
			 },
	_kvs = {val = {
						{k=12, v=12},
						{k=12, v=12},
						{k=12, v=12},
					},
			 type = "map",
			 ktype="i16",
			 vtype="i16",
			 },
	[1] = "_id",
	[2] = "_user",
	[3] = "_items",
	[4] = "_info",
	[5] = "_kvs",
}

function LoginReq:new()
	return ProtoCore:new(LoginReq)
end




local data = ProtoCore:pack(LoginReq:new(), "struct")

local hex = ""
for i = 1, #data do
	hex = hex .. string.format("%02x ",string.byte(data, i))
end
print(hex)


