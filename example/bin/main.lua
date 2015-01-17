--require
require("proto4z")
C2S_Pulse = 10000
S2C_Pulse = 10001
C2S_ECHO_REQ = 10002
S2C_ECHO_ACK = 10003

--echo pack

Protoz.register(S2C_ECHO_ACK, "EchoPack")

Protoz.EchoPack = {}
Protoz.EchoPack.__getName = "EchoPack"
Protoz.EchoPack[1] = {name = "_text", type = "string"}



-- 连接成功事件
function onConnect(sID)
	print("session is on connected. sID=" .. sID)
  local data = Protoz.encode({_text="test content"}, "EchoPack")
	summer.sendContent(sID, C2S_ECHO_REQ, data)
end
summer.registerConnect(onConnect)

-- 收到消息
function onMessage(sID, pID, content)
	--print("onMessage. sID=" .. sID .. ", pID=" .. pID )
  local echo = Protoz.decode(content, pID)
  --Protoz.dump(echo)

  local data = Protoz.encode({_text="test content"}, "EchoPack")
  summer.sendContent(sID, C2S_ECHO_REQ, data)
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
summer.logi("new connect id=" .. id)

--进入循环
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
while 1 do
	summer.runOnce()
--	summer.runOnce(true)
end


