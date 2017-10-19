 
Proto4z.MAX_SESSIONS = 5000--max session count 
Proto4z.SS_NONE = 0--不存在 
Proto4z.SS_CREATED = 1--已创建 
Proto4z.SS_WORKING = 10--working状态 
Proto4z.SS_DESTROY = 11--已销毁 
 
Proto4z.register(30000,"IntegerData") 
Proto4z.IntegerData = {} --测试 
Proto4z.IntegerData.__protoID = 30000 
Proto4z.IntegerData.__protoName = "IntegerData" 
Proto4z.IntegerData[1] = {name="_char", type="i8" }  
Proto4z.IntegerData[2] = {name="_uchar", type="ui8" }  
Proto4z.IntegerData[3] = {name="_short", type="i16" }  
Proto4z.IntegerData[4] = {name="_ushort", type="ui16" }  
Proto4z.IntegerData[5] = {name="_int", type="i32" }  
Proto4z.IntegerData[6] = {name="_uint", type="ui32" }  
Proto4z.IntegerData[7] = {name="_i64", type="i64" }  
Proto4z.IntegerData[8] = {name="_ui64", type="ui64" }  
 
Proto4z.register(30001,"FloatData") 
Proto4z.FloatData = {} --测试 
Proto4z.FloatData.__protoID = 30001 
Proto4z.FloatData.__protoName = "FloatData" 
Proto4z.FloatData[1] = {name="_float", type="float" }  
Proto4z.FloatData[2] = {name="_double", type="double" }  
 
Proto4z.register(30002,"StringData") 
Proto4z.StringData = {} --测试 
Proto4z.StringData.__protoID = 30002 
Proto4z.StringData.__protoName = "StringData" 
Proto4z.StringData[1] = {name="_string", type="string" }  
 
Proto4z.IntArray = {}  
Proto4z.IntArray.__protoName = "IntArray" 
Proto4z.IntArray.__protoDesc = "array" 
Proto4z.IntArray.__protoTypeV = "ui32" 
 
Proto4z.IntegerDataArray = {}  
Proto4z.IntegerDataArray.__protoName = "IntegerDataArray" 
Proto4z.IntegerDataArray.__protoDesc = "array" 
Proto4z.IntegerDataArray.__protoTypeV = "IntegerData" 
 
Proto4z.FloatDataArray = {}  
Proto4z.FloatDataArray.__protoName = "FloatDataArray" 
Proto4z.FloatDataArray.__protoDesc = "array" 
Proto4z.FloatDataArray.__protoTypeV = "FloatData" 
 
Proto4z.StringDataArray = {}  
Proto4z.StringDataArray.__protoName = "StringDataArray" 
Proto4z.StringDataArray.__protoDesc = "array" 
Proto4z.StringDataArray.__protoTypeV = "StringData" 
 
Proto4z.IntegerDataMap = {}  
Proto4z.IntegerDataMap.__protoName = "IntegerDataMap" 
Proto4z.IntegerDataMap.__protoDesc = "map" 
Proto4z.IntegerDataMap.__protoTypeK = "ui32" 
Proto4z.IntegerDataMap.__protoTypeV = "IntegerData" 
 
Proto4z.FloatDataMap = {}  
Proto4z.FloatDataMap.__protoName = "FloatDataMap" 
Proto4z.FloatDataMap.__protoDesc = "map" 
Proto4z.FloatDataMap.__protoTypeK = "double" 
Proto4z.FloatDataMap.__protoTypeV = "FloatData" 
 
Proto4z.StringDataMap = {}  
Proto4z.StringDataMap.__protoName = "StringDataMap" 
Proto4z.StringDataMap.__protoDesc = "map" 
Proto4z.StringDataMap.__protoTypeK = "string" 
Proto4z.StringDataMap.__protoTypeV = "StringData" 
 
Proto4z.register(30003,"EchoPack") 
Proto4z.EchoPack = {}  
Proto4z.EchoPack.__protoID = 30003 
Proto4z.EchoPack.__protoName = "EchoPack" 
Proto4z.EchoPack[1] = {name="_iarray", type="IntegerDataArray" }  
Proto4z.EchoPack[2] = {name="_farray", type="FloatDataArray" }  
Proto4z.EchoPack[3] = {name="_sarray", type="StringDataArray" }  
Proto4z.EchoPack[4] = {name="_imap", type="IntegerDataMap" }  
Proto4z.EchoPack[5] = {name="_fmap", type="FloatDataMap" }  
Proto4z.EchoPack[6] = {name="_smap", type="StringDataMap" }  
 
Proto4z.register(30004,"MoneyTree") 
Proto4z.MoneyTree = {} --摇钱树功能模块 
Proto4z.MoneyTree.__protoID = 30004 
Proto4z.MoneyTree.__protoName = "MoneyTree" 
Proto4z.MoneyTree[1] = {name="lastTime", type="ui32" } --最后一次执行时间 
Proto4z.MoneyTree[2] = {name="freeCount", type="ui32" } --今日剩余免费次数 
Proto4z.MoneyTree[3] = {name="payCount", type="ui32" } --今日已购买次数 
Proto4z.MoneyTree[4] = {name="statSum", type="ui32" } --历史总和 
Proto4z.MoneyTree[5] = {name="statCount", type="ui32" } --历史总次数 
 
Proto4z.register(30005,"SimplePack") 
Proto4z.SimplePack = {} --简单示例 
Proto4z.SimplePack.__protoID = 30005 
Proto4z.SimplePack.__protoName = "SimplePack" 
Proto4z.SimplePack[1] = {name="id", type="ui32" } --id, 对应数据库的结构为自增ID,key 
Proto4z.SimplePack[2] = {name="name", type="string" } --昵称, 唯一索引 
Proto4z.SimplePack[3] = {name="createTime", type="ui32" } --创建时间, 普通索引 
Proto4z.SimplePack[4] = {name="moneyTree", type="MoneyTree" }  
