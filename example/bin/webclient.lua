package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")
logw = summer.logw
logi = summer.logi
loge = summer.loge
print=summer.logd
-- 连接成功事件
local function whenLinked(sID, remoteIP, remotePort)
    print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
    --summer.webGet(sID, "/manual/index.php?id=1000", {})
    summer.webPost(sID, "/manual/index.php", nil, "id")
end
summer.whenLinked(whenLinked)

-- 收到消息
local function whenWebMessage(sID, cmd, head, body)
    Proto4z.dump(cmd)
    Proto4z.dump(head)
    Proto4z.dump(body)   
    summer.kick(sID)
end
summer.whenWebMessage(whenWebMessage)

-- 连接断开事件
local function whenClosed(sID, remoteIP, remotePort)
    print("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenClosed(whenClosed)

-- 脉冲
local function whenPulse(sID)
    print("session is on pulse. sID=" .. sID )
end
summer.whenPulse(whenPulse)

--启动网络
summer.start()

--连接服务器
local id = summer.addConnect("127.0.0.1", 8881, nil, nil,nil, 1 )
if id == nil then
    summer.logw("id == nil when addConnect")
end
print("new connect id=" .. id)

local startTick = summer.now()
--进入循环
--summer.run()
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() and summer.now() - startTick < 10*1000 do
end


