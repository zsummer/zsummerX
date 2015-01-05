
-- 连接成功事件
function onConnect(sID)
	print("session is on connected. sID=" .. sID)
	summer.sendContent(sID, 1001, "sssssss\0")
end
summer.registerConnect(onConnect)

-- 收到消息
function onMessage(sID, pID, content)
	print("onMessage. sID=" .. sID .. ", pID=" .. pID .. ", content:" .. content)
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


--[[

function re_print(t,prefix)
  for k,v in pairs(t) do
    if type(v) == "string" then
      print(string.format("%s => %s", prefix .. "." .. k,v))
  	elseif v == nil then
  		print(prefix .. "." .. k .. " ==> [" .. nil .. "]")
    else
      print(prefix .. "." .. k .. " ==> [" .. type(v) .. "]")
    end
    if type(v) == "table" and k ~= "_G" and k ~= "_G._G" and not v.package then
      re_print(v, "  " .. prefix .. "." .. k)
    end
  end
end

re_print(_G, "_G")

]]--
