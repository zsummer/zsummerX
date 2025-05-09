package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")



local lastTime = os.time()
local echoCount = 0

-- 收到消息
local function whenMessage(sID, pID, content)
    --print("whenMessage. sID=" .. sID .. ", pID=" .. pID )
    --Proto4z.putbin(content)
    local name = Proto4z.getName(pID)
    if name == nil then
        logw("unknown message id recv. pID=" .. pID)
    else
        --echo
        summer.sendContent(sID, pID, content)

        --Proto4z.dump(echo)
        --local echo = Proto4z.decode(content, name)
        echoCount = echoCount + 1
        if os.time() - lastTime >=5 then
            print("per second = " .. echoCount/5)
            echoCount = 0
            lastTime = os.time()
        end
    end

end
summer.whenMessage(whenMessage)


-- 连接成功事件
local function whenLinked(sID, remoteIP, remotePort)
    print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenLinked(whenLinked)
-- 连接断开事件
local function whenClosed(sID, remoteIP, remotePort)
    print("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
    --summer.stop();
end
summer.whenClosed(whenClosed)



--启动网络
summer.start()

--服务器监听
local id = summer.addListen("0.0.0.0",8881)
if id == nil then
    summer.logw("id == nil when addListen")
end
print("new accept id=" .. id)

local startTick = summer.now()
--进入循环
--summer.run()
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() and summer.now() - startTick < 10 do  

end



