local Proto = {}
function Proto:test()
	print("Proto:test")
end
function Proto:pack(val, tp)
	print("begin pack")
	-- base integer type
	if tp == "i8" then
		return string.pack("<i8", val)
	elseif tp == "ui8" then
		return string.pack("<I8", val)

	elseif tp == "i16" then
		return string.pack("<i16", val)
	elseif tp == "ui16" then
		return string.pack("<I16", val)	

	elseif tp == "i32" then
		return string.pack("<i32", val)
	elseif tp == "ui32" then
		return string.pack("<I32", val)	

	elseif tp == "i64" then
		return string.pack("<i64", val)
	elseif tp == "ui64" then
		return string.pack("<I64", val)	

	-- string type
	elseif tp == "string" then
		return string.pack("<s16", val)

	-- float type
	elseif tp == "float" then
		return string.pack("<f", val)
	elseif tp == "thenuble" then
		return string.pack("<d", val)

	-- user type
	else
		print(type(val))
	end
end


return Proto
