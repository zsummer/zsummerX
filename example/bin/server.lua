package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")



local lastTime = os.time()
local echoCount = 0

-- 收到消息
function onMessage(sID, pID, content)
	--print("onMessage. sID=" .. sID .. ", pID=" .. pID )
	--Protoz.putbin(content)
	local name = Protoz.getName(pID)
	local echo = Protoz.decode(content, name)
	--Protoz.dump(echo)
	summer.sendContent(sID, pID, content)

	echoCount = echoCount + 1
	if os.time() - lastTime >=5 then
		print("per second = " .. echoCount/5)
		echoCount = 0
		lastTime = os.time()
	end
end
summer.registerMessage(onMessage)




--启动网络
summer.start()

--连接服务器
local id = summer.addListen({ip="127.0.0.1", port=8081})
if id == nil then
	summer.logw("id == nil when addListen")
end
print("new accept id=" .. id)

--进入循环
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
while 1 do
	summer.runOnce()
--	summer.runOnce(true)
end


