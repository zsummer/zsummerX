-- proto4z core file
-- Auther: zhangyawei
-- date: 2015-01-12


Protoz = {}

-- base data pack
function Protoz:__pack(val, tp)
	if val == nil or tp == nil then
		error(debug.traceback())
	end
	--print(tp .. "[" .. val .. "]")
	-- integer type
	if     tp == "i8" or tp == "char" then
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
		error("unknown base data type when __pack it. val type=" .. type(val) .. "trace: " .. debug.traceback())
		return nil
	end
end


function Protoz:__encode(obj, protoName, data)
	if protoName == nil then
		error("ProtoName is nil. " .. debug.traceback())
	end
	local proto = Protoz[protoName]
	if proto == nil then
		error("unknown ProtoName[" .. protoName .. "] " .. debug.traceback())
	end

	for i=1, #proto do
		local protoType = proto[i]
		if type(protoType) ~= "table" then
			error("parse Proto error. protoName[" .. protoName .. "] " .. debug.traceback())
		end
		if type(protoType.name) ~= "string" or type(protoType.type) ~= "string" or type(protoType.vtype) ~= "string" then
			error("parse Proto error. proto[" .. protoType.name .. ":" .. protoType.type ..  "] " .. debug.traceback())
		end
		local val = obj[protoType.name]
		if val == nil then
			error("not found the dest object. name[" .. protoType.name .. "] " .. debug.traceback())
		end

		if protoType.type == "normal" then
			if type(val) ~= "table" then
				data.data = data.data .. Protoz:__pack(val, protoType.vtype)
			else
				Protoz:__encode(val, protoType.vtype, data)
			end
		elseif protoType.type == "array" then
			for j =1, #val do
				if type(val[j]) ~= "table" then
					data.data = data.data .. Protoz:__pack(val[j], protoType.vtype)
				else
					Protoz:__encode(val[j], protoType.vtype, data)
				end
			end
		elseif protoType.type == "dict" then
			for j =1, #val do
				if type(val[j].k) ~= "table" then
					data.data = data.data .. Protoz:__pack(val[j].k, protoType.ktype)
				else
					Protoz:__encode(val[j].k, protoType.ktype, data)
				end
				if type(val[j].v) ~= "table" then
					data.data = data.data .. Protoz:__pack(val[j].v, protoType.vtype)
				else
					Protoz:__encode(val[j].v, protoType.vtype, data)
				end
			end
		end
	end
end

function Protoz:encode(obj, protoName)
	local data = {data=""}
	Protoz:__encode(obj, protoName, data)
	return data.data
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
