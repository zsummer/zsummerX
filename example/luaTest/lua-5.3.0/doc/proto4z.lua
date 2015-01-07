local Proto = {}
function Proto:test()
	print("Proto:test")
end
function Proto:pack(val, tp)
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
	elseif tp == "table" then
		local str = ""
		for i = 1, #val do
			local v = val[val[i]]
			if v ~= nil then
				str = str .. Proto:pack(v.val, v.type)
			end
		end
		return str
	else
		print("unknown val type=" .. type(val))
	end
end


return Proto
