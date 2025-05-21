package.path =  "../../depends/proto4z/?.lua;" .. package.path
--require
require("proto4z")
require("TestProto")
logw = summer.logw
logi = summer.logi
loge = summer.loge
dump = Proto4z.dump
--dump = Proto4z.fastdump

--[[
local begin = summer.now()
for i = 0, 20*10000, 1 do
    summer.logt("ffffffffsdddddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", " a=", 1, ", b=", 2, ", c=", 32.234)
end
logi("fast 200*10000 logt write used=", summer.now()-begin)

local begin = summer.now()
for i = 0, 20*10000, 1 do
    summer.logt("ffffffffsdddddddddddddddddddddddddddddddddddddddddddddddddddddddddd" .. " a=" .. 1 .. ", b=" ..  2 ..", c=" .. 32.234)
end
logi("old 200*10000 logt write used=", summer.now()-begin)


exit(0)
]]--

--echo pack

local echo = {  _iarray = {{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64=1234565478,_ui64=13600000000005},{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="1234567812213123.2",_ui64="123"}},
                _farray = {{_float=2.235,_double=235.111},{_float=2.235,_double=235.111},},
                _sarray = {{_string="abcdefg"},{_string="abcdefg"},{_string="abcdefg"}},
                _imap = {[123]={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}, [223]={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64=1234567812213123,_ui64="12345678"}},
                _fmap = {[523.3]={_float=2.235,_double=235.111},["623"]={_float=2.235,_double=235.111}},
                _smap = {fadsfdas={_string="abcdefg"},ssss={_string="abcdefg"}},
                }

dump(echo)


-- 连接成功事件
local function whenLinked(sID, remoteIP, remotePort)
    print("session is on connected. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
    local data = Proto4z.encode(echo, "EchoPack")
    dump(echo)
    Proto4z.putbin(data)
    summer.sendContent(sID, Proto4z.EchoPack.__protoID, data)
end
summer.whenLinked(whenLinked)

local function whenLose(sID, remoteIP, remotePort)
    print("session is on lose. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
    --summer.stop();
end
summer.whenClosed(whenLose)


-- 收到消息
local function whenMessage(sID, pID, content)
    --print("whenMessage. sID=" .. sID .. ", pID=" .. pID )
    --Proto4z.putbin(content)
    local name = Proto4z.getName(pID)
    if name == nil then
        logw("unknown message id recv. pID=" .. pID)
    elseif (true) then
        summer.sendContent(sID, pID, content)
    else
            local echo = Proto4z.decode(content, name)
            --dump(echo)
            local data = Proto4z.encode(echo, "EchoPack")
            summer.sendContent(sID, Proto4z.EchoPack.__protoID, data)
            --local data = Proto4z.pack(echo, "EchoPack")
            --summer.sendData(sID, data)
    end

end
summer.whenMessage(whenMessage)

-- 连接断开事件
local function whenClosed(sID, remoteIP, remotePort)
    logi("session is on disconnect. sID=" .. sID .. ", remoteIP=" .. remoteIP .. ", remotePort=" .. remotePort)
end
summer.whenClosed(whenClosed)

-- 脉冲
local function whenPulse(sID)
    logi("session is on pulse. sID=" .. sID )
end
summer.whenPulse(whenPulse)

--启动网络
summer.start()
for i=1, 2, 1 do
    --连接服务器
    local id = summer.addConnect("127.0.0.1", 8881, nil, 5)
    if id == nil then
        summer.logw("id == nil when addConnect")
    end
    logi("new connect id=" .. id)

end



local startTick = summer.now()
--进入循环
--summer.run()
--如果嵌入其他程序 例如cocos2dx, 可以吧runOnce设置true然后放入update中.
--while summer.runOnce(true) do
while summer.runOnce() and summer.now() - startTick < 10 do    

end


