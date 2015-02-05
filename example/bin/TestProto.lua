Protoz.EC_SUCCESS = 0--success 
Protoz.EC_ERROR = 1--error 
Protoz.EC_INVALID_PARAM = 1000--invalid param 
Protoz.EC_UNKNOWN_ID = 1001--unknwon id 
 
Protoz.TestIntegerData = {} --测试 
Protoz.TestIntegerData.__getName = "TestIntegerData" 
Protoz.TestIntegerData[1] = {name="_char", type="i8" }  
Protoz.TestIntegerData[2] = {name="_uchar", type="ui8" }  
Protoz.TestIntegerData[3] = {name="_short", type="i16" }  
Protoz.TestIntegerData[4] = {name="_ushort", type="ui16" }  
Protoz.TestIntegerData[5] = {name="_int", type="i32" }  
Protoz.TestIntegerData[6] = {name="_uint", type="ui32" }  
Protoz.TestIntegerData[7] = {name="_i64", type="i64" }  
Protoz.TestIntegerData[8] = {name="_ui128", type="ui64", del = true }  
Protoz.TestIntegerData[9] = {name="_ui64", type="ui64" }  
 
Protoz.TestFloatData = {} --测试 
Protoz.TestFloatData.__getName = "TestFloatData" 
Protoz.TestFloatData[1] = {name="_float", type="float" }  
Protoz.TestFloatData[2] = {name="_double", type="double" }  
 
Protoz.TestStringData = {} --测试 
Protoz.TestStringData.__getName = "TestStringData" 
Protoz.TestStringData[1] = {name="_string", type="string" }  
 
Protoz.IntArray = {}  
Protoz.IntArray.__getName = "IntArray" 
Protoz.IntArray.__getDesc = "array" 
Protoz.IntArray.__getTypeV = "ui32" 
 
Protoz.TestIntegerDataArray = {}  
Protoz.TestIntegerDataArray.__getName = "TestIntegerDataArray" 
Protoz.TestIntegerDataArray.__getDesc = "array" 
Protoz.TestIntegerDataArray.__getTypeV = "TestIntegerData" 
 
Protoz.TestFloatDataArray = {}  
Protoz.TestFloatDataArray.__getName = "TestFloatDataArray" 
Protoz.TestFloatDataArray.__getDesc = "array" 
Protoz.TestFloatDataArray.__getTypeV = "TestFloatData" 
 
Protoz.TestStringDataArray = {}  
Protoz.TestStringDataArray.__getName = "TestStringDataArray" 
Protoz.TestStringDataArray.__getDesc = "array" 
Protoz.TestStringDataArray.__getTypeV = "TestStringData" 
 
Protoz.TestIntegerDataMap = {}  
Protoz.TestIntegerDataMap.__getName = "TestIntegerDataMap" 
Protoz.TestIntegerDataMap.__getDesc = "map" 
Protoz.TestIntegerDataMap.__getTypeK = "string" 
Protoz.TestIntegerDataMap.__getTypeV = "TestIntegerData" 
 
Protoz.TestFloatDataMap = {}  
Protoz.TestFloatDataMap.__getName = "TestFloatDataMap" 
Protoz.TestFloatDataMap.__getDesc = "map" 
Protoz.TestFloatDataMap.__getTypeK = "string" 
Protoz.TestFloatDataMap.__getTypeV = "TestFloatData" 
 
Protoz.TestStringDataMap = {}  
Protoz.TestStringDataMap.__getName = "TestStringDataMap" 
Protoz.TestStringDataMap.__getDesc = "map" 
Protoz.TestStringDataMap.__getTypeK = "string" 
Protoz.TestStringDataMap.__getTypeV = "TestStringData" 
 
Protoz.register(30000,"P2P_EchoPack") 
Protoz.P2P_EchoPack = {}  
Protoz.P2P_EchoPack.__getID = 30000 
Protoz.P2P_EchoPack.__getName = "P2P_EchoPack" 
Protoz.P2P_EchoPack[1] = {name="_iarray", type="TestIntegerDataArray" }  
Protoz.P2P_EchoPack[2] = {name="_farray", type="TestFloatDataArray" }  
Protoz.P2P_EchoPack[3] = {name="_sarray", type="TestStringDataArray" }  
Protoz.P2P_EchoPack[4] = {name="_imap", type="TestIntegerDataMap" }  
Protoz.P2P_EchoPack[5] = {name="_fmap", type="TestFloatDataMap" }  
Protoz.P2P_EchoPack[6] = {name="_smap", type="TestStringDataMap" }  
 
Protoz.register(30001,"C2S_Pulse") 
Protoz.C2S_Pulse = {}  
Protoz.C2S_Pulse.__getID = 30001 
Protoz.C2S_Pulse.__getName = "C2S_Pulse" 
 
Protoz.register(30002,"S2C_Pulse") 
Protoz.S2C_Pulse = {}  
Protoz.S2C_Pulse.__getID = 30002 
Protoz.S2C_Pulse.__getName = "S2C_Pulse" 
