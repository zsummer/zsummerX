package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")
logw = summer.logw
logi = summer.logi
loge = summer.loge

--echo pack

local echo = {  _iarray = {{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"},{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}},
				_farray = {{_float=2.235,_double=235.111},{_float=2.235,_double=235.111},},
				_sarray = {{_string="abcdefg"},{_string="abcdefg"},{_string="abcdefg"}},
				_imap = {{k="123", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}, {k="223", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}},
				_fmap = {{k="523", v={_float=2.235,_double=235.111}},{k="623", v={_float=2.235,_double=235.111}}},
				_smap = {{k="723", v={_string="abcdefg"}},{k="823", v={_string="abcdefg"}}},
				}

-- 连接成功事件
function onConnect(sID, remoteIP, remotePort)
	print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
	local data = Proto4z.encode(echo, "EchoPack")
	Proto4z.dump(echo)
	Proto4z.putbin(data)
	summer.sendContent(sID, Proto4z.EchoPack.__getID, data)
end
summer.registerConnect(onConnect)

-- 收到消息
function onMessage(sID, pID, content)
	--print("onMessage. sID=" .. sID .. ", pID=" .. pID )
	--Proto4z.putbin(content)
	local name = Proto4z.getName(pID)
	if name == nil then
		logw("unknown message id recv. pID=" .. pID)
	else
			local echo = Proto4z.decode(content, name)
			--Proto4z.dump(echo)
			local data = Proto4z.encode(echo, "EchoPack")
			summer.sendContent(sID, Proto4z.EchoPack.__getID, data)
	end

end
summer.registerMessage(onMessage)

-- 连接断开事件
function onDisconnect(sID, remoteIP, remotePort)
	print("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.registerDisconnect(onDisconnect)


--启动网络
summer.start()

--连接服务器
local id = summer.addConnect("127.0.0.1", 8081, nil, 10)
if id == nil then
	summer.logw("id == nil when addConnect")
end
print("new connect id=" .. id)

--进入循环
--summer.run()
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() do
end


