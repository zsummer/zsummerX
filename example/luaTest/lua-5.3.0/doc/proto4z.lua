
-- 所有协议都在该table
Protoz = {}

function Protoz:__pack(val, tp)
	print("begin pack")
	-- base integer type
	if tp == "i8" or tp == "char" then
		return string.pack("<i1", val)
	elseif tp == "ui8" or tp == "unsigned char" then
		return string.pack("<I1", val)

	elseif tp == "i16" or tp == "short" then
		return string.pack("<i2", val)
	elseif tp == "ui16" or tp == "unsigned short" then
		return string.pack("<I2", val)	

	elseif tp == "i32" or tp == "int" then
		return string.pack("<i4", val)
	elseif tp == "ui32" or tp == "unsigned int" then
		return string.pack("<I4", val)	

	elseif tp == "i64" or tp == "long long" then
		return string.pack("<i8", val)
	elseif tp == "ui64" or tp == "unsigned long long" then
		return string.pack("<I8", val)	

	-- string type
	elseif tp == "string" then
		return string.pack("<s2", val)

	-- float type
	elseif tp == "float" then
		return string.pack("<f", val)
	elseif tp == "double" then
		return string.pack("<d", val)

	-- error
	else
		print("unknown val type=" .. type(val))
		return nil
	end
end

function Protoz:__encode(obj, protoName, data)
	local proto = Protoz[protoName]
	if proto = nil then
		return nil
	end

	for i=1, #proto do
		local pt = proto[i]
		local val = obj[pt.name]
		if pt.type == "normal" then
			if type(val) ~= "table" then
				data.data = data.data .. Protoz:__pack(val, pt.vtype)
			else
				Protoz:__encode(val, pt.vtype, data)
			end
		elseif pt.type == "array" then
			for j =1, #val do
				if type(val[i]) ~= "table" then
					data.data = data.data .. Protoz:__pack(val[i], pt.vtype)
				else
					Protoz:__encode(val[i], pt.vtype, data)
				end
			end
		end
	end
end

function Protoz:encode(obj, protoName)
	local data = {data=""}
	Protoz:__encode(obj, protoName, data)
	return data
end


function Protoz:new(tb)
    local recursive = {} -- log recursive table
    local function _copy(tb)
        if type(tb) ~= "table" then
            return tb
        elseif recursive[tb] then
            return recursive[tb]
        end
        local newtb = {}
        recursive[tb] = newtb
        for k, v in pairs(tb) do
            newtb[_copy(k)] = _copy(v)
        end
        return setmetatable(newtb, getmetatable(tb))
    end
    return _copy(tb)
end


return ProtoCore
