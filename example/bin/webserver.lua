package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")
print=summer.logd


local lastTime = os.time()
local echoCount = 0

-- 收到消息
local function whenWebMessage(sID, cmd, head, body)
    Proto4z.dump(cmd)
    Proto4z.dump(head)
    Proto4z.dump(body)
    Proto4z.dump(body)
    summer.webResponse(sID, 200, {aaa="bbb"}, body)
end
summer.whenWebMessage(whenWebMessage)


-- 连接成功事件
local function whenLinked(sID, remoteIP, remotePort)
    print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenLinked(whenLinked)
-- 连接断开事件
local function whenClosed(sID, remoteIP, remotePort)
    print("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenClosed(whenClosed)


--启动网络
summer.start()

--服务器监听
local id = summer.addListen("0.0.0.0",8881,"", nil, nil, 1)
if id == nil then
    summer.logw("id == nil when addListen")
end
print("new accept id=" .. id)

local startTick = summer.now()
--进入循环
--summer.run()
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() and summer.now() - startTick < 10*1000 do
end



