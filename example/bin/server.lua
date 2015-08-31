package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")



local lastTime = os.time()
local echoCount = 0

-- 收到消息
function onMessage(sID, pID, content)
	--print("onMessage. sID=" .. sID .. ", pID=" .. pID )
	--Proto4z.putbin(content)
	local name = Proto4z.getName(pID)
	if name == nil then
		logw("unknown message id recv. pID=" .. pID)
	else
		--echo
		summer.sendContent(sID, pID, content)

		--Proto4z.dump(echo)
		local echo = Proto4z.decode(content, name)
		echoCount = echoCount + 1
		if os.time() - lastTime >=5 then
			print("per second = " .. echoCount/5)
			echoCount = 0
			lastTime = os.time()
		end
	end

end
summer.whenMessage(onMessage)


-- 连接成功事件
function whenLinked(sID, remoteIP, remotePort)
	print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenLinked(whenLinked)
-- 连接断开事件
function whenClosed(sID, remoteIP, remotePort)
	print("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenClosed(whenClosed)


--启动网络
summer.start()

--连接服务器
local id = summer.addListen("127.0.0.1",8081)
if id == nil then
	summer.logw("id == nil when addListen")
end
print("new accept id=" .. id)

--进入循环
--summer.run()

--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() do
end


