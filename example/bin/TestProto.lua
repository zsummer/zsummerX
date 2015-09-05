Proto4z.EC_SUCCESS = 0--success 
Proto4z.EC_ERROR = 1--error 
Proto4z.EC_INVALID_PARAM = 1000--invalid param 
Proto4z.EC_UNKNOWN_ID = 1001--unknwon id 
 
Proto4z.TestIntegerData = {} --测试 
Proto4z.TestIntegerData.__getName = "TestIntegerData" 
Proto4z.TestIntegerData[1] = {name="_char", type="i8" }  
Proto4z.TestIntegerData[2] = {name="_uchar", type="ui8" }  
Proto4z.TestIntegerData[3] = {name="_short", type="i16" }  
Proto4z.TestIntegerData[4] = {name="_ushort", type="ui16" }  
Proto4z.TestIntegerData[5] = {name="_int", type="i32" }  
Proto4z.TestIntegerData[6] = {name="_uint", type="ui32" }  
Proto4z.TestIntegerData[7] = {name="_i64", type="i64" }  
Proto4z.TestIntegerData[8] = {name="_ui128", type="ui64", del = true }  
Proto4z.TestIntegerData[9] = {name="_ui64", type="ui64" }  
 
Proto4z.TestFloatData = {} --测试 
Proto4z.TestFloatData.__getName = "TestFloatData" 
Proto4z.TestFloatData[1] = {name="_float", type="float" }  
Proto4z.TestFloatData[2] = {name="_double", type="double" }  
 
Proto4z.TestStringData = {} --测试 
Proto4z.TestStringData.__getName = "TestStringData" 
Proto4z.TestStringData[1] = {name="_string", type="string" }  
 
Proto4z.IntArray = {}  
Proto4z.IntArray.__getName = "IntArray" 
Proto4z.IntArray.__getDesc = "array" 
Proto4z.IntArray.__getTypeV = "ui32" 
 
Proto4z.TestIntegerDataArray = {}  
Proto4z.TestIntegerDataArray.__getName = "TestIntegerDataArray" 
Proto4z.TestIntegerDataArray.__getDesc = "array" 
Proto4z.TestIntegerDataArray.__getTypeV = "TestIntegerData" 
 
Proto4z.TestFloatDataArray = {}  
Proto4z.TestFloatDataArray.__getName = "TestFloatDataArray" 
Proto4z.TestFloatDataArray.__getDesc = "array" 
Proto4z.TestFloatDataArray.__getTypeV = "TestFloatData" 
 
Proto4z.TestStringDataArray = {}  
Proto4z.TestStringDataArray.__getName = "TestStringDataArray" 
Proto4z.TestStringDataArray.__getDesc = "array" 
Proto4z.TestStringDataArray.__getTypeV = "TestStringData" 
 
Proto4z.TestIntegerDataMap = {}  
Proto4z.TestIntegerDataMap.__getName = "TestIntegerDataMap" 
Proto4z.TestIntegerDataMap.__getDesc = "map" 
Proto4z.TestIntegerDataMap.__getTypeK = "string" 
Proto4z.TestIntegerDataMap.__getTypeV = "TestIntegerData" 
 
Proto4z.TestFloatDataMap = {}  
Proto4z.TestFloatDataMap.__getName = "TestFloatDataMap" 
Proto4z.TestFloatDataMap.__getDesc = "map" 
Proto4z.TestFloatDataMap.__getTypeK = "string" 
Proto4z.TestFloatDataMap.__getTypeV = "TestFloatData" 
 
Proto4z.TestStringDataMap = {}  
Proto4z.TestStringDataMap.__getName = "TestStringDataMap" 
Proto4z.TestStringDataMap.__getDesc = "map" 
Proto4z.TestStringDataMap.__getTypeK = "string" 
Proto4z.TestStringDataMap.__getTypeV = "TestStringData" 
 
Proto4z.register(30000,"EchoPack") 
Proto4z.EchoPack = {}  
Proto4z.EchoPack.__getID = 30000 
Proto4z.EchoPack.__getName = "EchoPack" 
Proto4z.EchoPack[1] = {name="_iarray", type="TestIntegerDataArray" }  
Proto4z.EchoPack[2] = {name="_farray", type="TestFloatDataArray" }  
Proto4z.EchoPack[3] = {name="_sarray", type="TestStringDataArray" }  
Proto4z.EchoPack[4] = {name="_imap", type="TestIntegerDataMap" }  
Proto4z.EchoPack[5] = {name="_fmap", type="TestFloatDataMap" }  
Proto4z.EchoPack[6] = {name="_smap", type="TestStringDataMap" }  
 
Proto4z.register(30001,"Pulse") 
Proto4z.Pulse = {}  
Proto4z.Pulse.__getID = 30001 
Proto4z.Pulse.__getName = "Pulse" 
