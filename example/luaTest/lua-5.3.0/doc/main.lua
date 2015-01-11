
require("proto4z")

Protoz.InvalidUserID = 0xffffffff

Protoz.EC_SUCCESS = 0
Protoz.EC_ERROR = 1

Protoz.Hero = {}
Protoz.Hero.__getName = "Hero"
Protoz.Hero.[1] = {name = "_name", type = "string"}

Protoz.UserInfo = {}
Protoz.UserInfo.__getName = "UserInfo"
Protoz.UserInfo[1] = {name = "_uID", type = "normal", vtype = "ui32"}
Protoz.UserInfo[2] = {name = "_user", type = "normal", vtype = "string"}
Protoz.UserInfo[3] = {name = "_items", type = "array", vtype = "ui32"}
Protoz.UserInfo[4] = {name = "_heros", type = "array", vtype = "Hero"}


Protoz.LoginResult = {}
Protoz.LoginResult.__getName = "LoginResult"
Protoz.LoginResult[1] = {name = "_retCode", type = "normal", vtype = "ui32"}
Protoz.LoginResult[2] = {name = "_info", type = "normal", vtype = "UserInfo"}


local result = {_retCode = 0,
					_info = {_uID = 100, _user = "username", _items = {1,2,3}, _heros = {{_name = "hero1"},{_name="hero2"}} } 
				}


local data = Protoz.encode(result, "LoginResult")


local hex = ""
for i = 1, #data do
	hex = hex .. string.format("%02x ",string.byte(data, i))
end
print(hex)


