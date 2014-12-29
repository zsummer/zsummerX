require "summer"


function onConnect(sID)
	print("session is on connected. sID=" .. sID)
end
summer.registConnect(onConnect)
summer.start()
local id = summer.addConnect({ip="127.0.0.1", port=8081})
if id == nil then
	summer.logw("id == nil when addConnect")
end
summer.logi("new connect id=" .. id)

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
