--[[
/*
 * proto4z License
 * -----------
 * 
 * proto4z is licensed under the terms of the MIT license reproduced below.
 * This means that proto4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013-2015 YaweiZhang <yawei.zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */
 ]]--

-- proto4z test file
-- Author: zhangyawei
-- mail:yawei.zhang@foxmail.com
-- date: 2015-01-12

package.path =package.path ..  ";../../?.lua;../genCode/lua/?.lua"
--require
require("proto4z")
require("TestProto")
dump = Proto4z.dump
local proto = Proto4z

local echo = {  _iarray = {{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64=12345678.2,_ui64=12345678},{_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="123456789000000.2",_ui64="1123122345678.0"}},
				_farray = {{_float=2.235,_double=235.111},{_float=2.235,_double=235.111},},
				_sarray = {{_string="abcdefg"},{_string="abcdefg"},{_string="abcdefg"}},
				_imap = {{k="123", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}, {k="223", v={_char=1,_uchar=2,_short=3,_ushort=4,_int=5,_uint=6,_i64="12345678",_ui64="12345678"}}},
				_fmap = {{k="523", v={_float=2.235,_double=235.111}},{k="623", v={_float=2.235,_double=235.111}}},
				_smap = {{k="523", v={_string="abcdefg"}},{k="523", v={_string="abcdefg"}}},
				}

function process(echo, isDebug)
	if isDebug then dump(echo, "process") end
	local data = proto.encode(echo, "EchoPack")
	if isDebug then dump(data, "encode") end
	local dr = proto.decode(data, proto.getName(proto.EchoPack.__getID))
	if isDebug then
		dump(dr, "decode")
	end
end

process(nil, true)
process({}, true)
process({_iarray={{}}, _farray={{}}, _fmap={{}}}, true)
local now = Proto4zUtil.now()
for i=1, 1000 do
	process(echo)
end
print("used time=" .. (Proto4zUtil.now() - now))













