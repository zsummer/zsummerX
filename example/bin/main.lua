package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")


--echo pack

local echo = {  _iarray = {{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"},{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}},
				_farray = {{_float=2.235,_double=235.111},{_float=2.235,_double=235.111},},
				_sarray = {{_string="abcdefg"},{_string="abcdefg"},{_string="abcdefg"}},
				_imap = {{k="123", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}, {k="223", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}},
				_fmap = {{k="523", v={_float=2.235,_double=235.111}},{k="623", v={_float=2.235,_double=235.111}}},
				_smap = {{k="723", v={_string="abcdefg"}},{k="823", v={_string="abcdefg"}}},
				}

-- 连接成功事件
function onConnect(sID)
	print("session is on connected. sID=" .. sID)
	local data = Protoz.encode(echo, "P2P_EchoPack")
	Protoz.dump(echo)
	Protoz.putbin(data)
	summer.sendContent(sID, Protoz.P2P_EchoPack.__getID, data)
end
summer.registerConnect(onConnect)

-- 收到消息
function onMessage(sID, pID, content)
	--print("onMessage. sID=" .. sID .. ", pID=" .. pID )
	--Protoz.putbin(content)
	local name = Protoz.getName(pID)
	local echo = Protoz.decode(content, name)
	--Protoz.dump(echo)
	local data = Protoz.encode(echo, "P2P_EchoPack")
	summer.sendContent(sID, Protoz.P2P_EchoPack.__getID, data)
end
summer.registerMessage(onMessage)

-- 连接断开事件
function onDisconnect(sID)
	print("session is on disconnect. sID=" .. sID)
end
summer.registerDisconnect(onDisconnect)


--启动网络
summer.start()

--连接服务器
local id = summer.addConnect({ip="127.0.0.1", port=8081, reconnect=2})
if id == nil then
	summer.logw("id == nil when addConnect")
end
print("new connect id=" .. id)

--进入循环
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
while 1 do
	summer.runOnce()
--	summer.runOnce(true)
end


