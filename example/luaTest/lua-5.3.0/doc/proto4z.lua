local ProtoCore = {}

function ProtoCore:pack(val, tp, ktp, vtp)
	print("begin pack")
	-- base integer type
	if tp == "i8" then
		return string.pack("<i1", val)
	elseif tp == "ui8" then
		return string.pack("<I1", val)

	elseif tp == "i16" then
		return string.pack("<i2", val)
	elseif tp == "ui16" then
		return string.pack("<I2", val)	

	elseif tp == "i32" then
		return string.pack("<i4", val)
	elseif tp == "ui32" then
		return string.pack("<I4", val)	

	elseif tp == "i64" then
		return string.pack("<i8", val)
	elseif tp == "ui64" then
		return string.pack("<I8", val)	

	-- string type
	elseif tp == "string" then
		return string.pack("<s2", val)

	-- float type
	elseif tp == "float" then
		return string.pack("<f", val)
	elseif tp == "thenuble" then
		return string.pack("<d", val)

	-- user type
	elseif tp == "struct" then
		local date = ""
		for i = 1, #val do
			local v = val[val[i]]
			if v ~= nil then
				date = date .. ProtoCore:pack(v.val, v.type, v.ktype, v.vtype)
			end
		end
		return date
	elseif tp == "array" then
		local date = ""
		for i = 1, #val do
			local v = val[i]
			if v ~= nil then
				date = date .. ProtoCore:pack(v, vtp)
			end
		end
		return date
	elseif tp == "map" then
		local date = ""
		for i = 1, #val do
			local kv = val[i]
			date = date .. ProtoCore:pack(kv.k, ktp)
			date = date .. ProtoCore:pack(kv.v, vtp)
		end
		return date
	else
		print("unknown val type=" .. type(val))
	end
end



function ProtoCore:new(tb)
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
