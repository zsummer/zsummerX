 
namespace Proto4z  
{ 
 
    public class MAX_SESSIONS //max session count  
    { 
        public const ushort value = 5000;  
    } 
 
    public enum ServiceStatus : ushort 
    { 
        SS_NONE = 0, //不存在  
        SS_CREATED = 1, //已创建  
        SS_WORKING = 10, //working状态  
        SS_DESTROY = 11, //已销毁  
    }; 
 
    public class IntegerData: Proto4z.IProtoObject //测试  
    {     
        //proto id   
        public const ushort protoID = 30000;  
        static public ushort getProtoID() { return 30000; } 
        static public string getProtoName() { return "IntegerData"; } 
        //members   
        public char _char;  
        public byte _uchar;  
        public short _short;  
        public ushort _ushort;  
        public int _int;  
        public uint _uint;  
        public long _i64;  
        public ulong _ui64;  
        public IntegerData()  
        { 
            _char = '\0';  
            _uchar = 0;  
            _short = 0;  
            _ushort = 0;  
            _int = 0;  
            _uint = 0;  
            _i64 = 0;  
            _ui64 = 0;  
        } 
        public IntegerData(char _char, byte _uchar, short _short, ushort _ushort, int _int, uint _uint, long _i64, ulong _ui64) 
        { 
            this._char = _char; 
            this._uchar = _uchar; 
            this._short = _short; 
            this._ushort = _ushort; 
            this._int = _int; 
            this._uint = _uint; 
            this._i64 = _i64; 
            this._ui64 = _ui64; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            data.AddRange(Proto4z.BaseProtoObject.encodeI8(this._char)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI8(this._uchar)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeI16(this._short)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI16(this._ushort)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeI32(this._int)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this._uint)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeI64(this._i64)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI64(this._ui64)); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this._char = Proto4z.BaseProtoObject.decodeI8(binData, ref pos); 
            this._uchar = Proto4z.BaseProtoObject.decodeUI8(binData, ref pos); 
            this._short = Proto4z.BaseProtoObject.decodeI16(binData, ref pos); 
            this._ushort = Proto4z.BaseProtoObject.decodeUI16(binData, ref pos); 
            this._int = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            this._uint = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this._i64 = Proto4z.BaseProtoObject.decodeI64(binData, ref pos); 
            this._ui64 = Proto4z.BaseProtoObject.decodeUI64(binData, ref pos); 
            return pos; 
        } 
    } 
 
    public class FloatData: Proto4z.IProtoObject //测试  
    {     
        //proto id   
        public const ushort protoID = 30001;  
        static public ushort getProtoID() { return 30001; } 
        static public string getProtoName() { return "FloatData"; } 
        //members   
        public float _float;  
        public double _double;  
        public FloatData()  
        { 
            _float = 0.0f;  
            _double = 0.0;  
        } 
        public FloatData(float _float, double _double) 
        { 
            this._float = _float; 
            this._double = _double; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            data.AddRange(Proto4z.BaseProtoObject.encodeSingle(this._float)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeDouble(this._double)); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this._float = Proto4z.BaseProtoObject.decodeSingle(binData, ref pos); 
            this._double = Proto4z.BaseProtoObject.decodeDouble(binData, ref pos); 
            return pos; 
        } 
    } 
 
    public class StringData: Proto4z.IProtoObject //测试  
    {     
        //proto id   
        public const ushort protoID = 30002;  
        static public ushort getProtoID() { return 30002; } 
        static public string getProtoName() { return "StringData"; } 
        //members   
        public string _string;  
        public StringData()  
        { 
            _string = "";  
        } 
        public StringData(string _string) 
        { 
            this._string = _string; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            data.AddRange(Proto4z.BaseProtoObject.encodeString(this._string)); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this._string = Proto4z.BaseProtoObject.decodeString(binData, ref pos); 
            return pos; 
        } 
    } 
 
 
    public class IntArray : System.Collections.Generic.List<uint>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            for (int i = 0; i < this.Count; i++ ) 
            { 
                ret.AddRange(Proto4z.BaseProtoObject.encodeUI32(this[i]));  
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    this.Add(Proto4z.BaseProtoObject.decodeUI32(binData, ref pos)); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class IntegerDataArray : System.Collections.Generic.List<IntegerData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            for (int i = 0; i < this.Count; i++ ) 
            { 
                ret.AddRange(this[i].__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var data = new IntegerData(); 
                    data.__decode(binData, ref pos); 
                    this.Add(data); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class FloatDataArray : System.Collections.Generic.List<FloatData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            for (int i = 0; i < this.Count; i++ ) 
            { 
                ret.AddRange(this[i].__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var data = new FloatData(); 
                    data.__decode(binData, ref pos); 
                    this.Add(data); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class StringDataArray : System.Collections.Generic.List<StringData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            for (int i = 0; i < this.Count; i++ ) 
            { 
                ret.AddRange(this[i].__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var data = new StringData(); 
                    data.__decode(binData, ref pos); 
                    this.Add(data); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class IntegerDataMap : System.Collections.Generic.Dictionary<uint, IntegerData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            foreach(var kv in this) 
            { 
                ret.AddRange(Proto4z.BaseProtoObject.encodeUI32(kv.Key)); 
                ret.AddRange(kv.Value.__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var key = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
                    var val = new IntegerData(); 
                    val.__decode(binData, ref pos); 
                    this.Add(key, val); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class FloatDataMap : System.Collections.Generic.Dictionary<double, FloatData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            foreach(var kv in this) 
            { 
                ret.AddRange(Proto4z.BaseProtoObject.encodeDouble(kv.Key)); 
                ret.AddRange(kv.Value.__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var key = Proto4z.BaseProtoObject.decodeDouble(binData, ref pos); 
                    var val = new FloatData(); 
                    val.__decode(binData, ref pos); 
                    this.Add(key, val); 
                } 
            } 
            return pos; 
        } 
    } 
 
 
    public class StringDataMap : System.Collections.Generic.Dictionary<string, StringData>, Proto4z.IProtoObject  
    { 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var ret = new System.Collections.Generic.List<byte>(); 
            int len = (int)this.Count; 
            ret.AddRange(Proto4z.BaseProtoObject.encodeI32(len)); 
            foreach(var kv in this) 
            { 
                ret.AddRange(Proto4z.BaseProtoObject.encodeString(kv.Key)); 
                ret.AddRange(kv.Value.__encode()); 
            } 
            return ret; 
        } 
 
        public int __decode(byte[] binData, ref int pos) 
        { 
            int len = Proto4z.BaseProtoObject.decodeI32(binData, ref pos); 
            if(len > 0) 
            { 
                for (int i=0; i<len; i++) 
                { 
                    var key = Proto4z.BaseProtoObject.decodeString(binData, ref pos); 
                    var val = new StringData(); 
                    val.__decode(binData, ref pos); 
                    this.Add(key, val); 
                } 
            } 
            return pos; 
        } 
    } 
 
    public class EchoPack: Proto4z.IProtoObject 
    {     
        //proto id   
        public const ushort protoID = 30003;  
        static public ushort getProtoID() { return 30003; } 
        static public string getProtoName() { return "EchoPack"; } 
        //members   
        public IntegerDataArray _iarray;  
        public FloatDataArray _farray;  
        public StringDataArray _sarray;  
        public IntegerDataMap _imap;  
        public FloatDataMap _fmap;  
        public StringDataMap _smap;  
        public EchoPack()  
        { 
            _iarray = new IntegerDataArray();  
            _farray = new FloatDataArray();  
            _sarray = new StringDataArray();  
            _imap = new IntegerDataMap();  
            _fmap = new FloatDataMap();  
            _smap = new StringDataMap();  
        } 
        public EchoPack(IntegerDataArray _iarray, FloatDataArray _farray, StringDataArray _sarray, IntegerDataMap _imap, FloatDataMap _fmap, StringDataMap _smap) 
        { 
            this._iarray = _iarray; 
            this._farray = _farray; 
            this._sarray = _sarray; 
            this._imap = _imap; 
            this._fmap = _fmap; 
            this._smap = _smap; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            if (this._iarray == null) this._iarray = new IntegerDataArray(); 
            data.AddRange(this._iarray.__encode()); 
            if (this._farray == null) this._farray = new FloatDataArray(); 
            data.AddRange(this._farray.__encode()); 
            if (this._sarray == null) this._sarray = new StringDataArray(); 
            data.AddRange(this._sarray.__encode()); 
            if (this._imap == null) this._imap = new IntegerDataMap(); 
            data.AddRange(this._imap.__encode()); 
            if (this._fmap == null) this._fmap = new FloatDataMap(); 
            data.AddRange(this._fmap.__encode()); 
            if (this._smap == null) this._smap = new StringDataMap(); 
            data.AddRange(this._smap.__encode()); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this._iarray = new IntegerDataArray(); 
            this._iarray.__decode(binData, ref pos); 
            this._farray = new FloatDataArray(); 
            this._farray.__decode(binData, ref pos); 
            this._sarray = new StringDataArray(); 
            this._sarray.__decode(binData, ref pos); 
            this._imap = new IntegerDataMap(); 
            this._imap.__decode(binData, ref pos); 
            this._fmap = new FloatDataMap(); 
            this._fmap.__decode(binData, ref pos); 
            this._smap = new StringDataMap(); 
            this._smap.__decode(binData, ref pos); 
            return pos; 
        } 
    } 
 
    public class MoneyTree: Proto4z.IProtoObject //摇钱树功能模块  
    {     
        //proto id   
        public const ushort protoID = 30004;  
        static public ushort getProtoID() { return 30004; } 
        static public string getProtoName() { return "MoneyTree"; } 
        //members   
        public uint lastTime; //最后一次执行时间  
        public uint freeCount; //今日剩余免费次数  
        public uint payCount; //今日已购买次数  
        public uint statSum; //历史总和  
        public uint statCount; //历史总次数  
        public MoneyTree()  
        { 
            lastTime = 0;  
            freeCount = 0;  
            payCount = 0;  
            statSum = 0;  
            statCount = 0;  
        } 
        public MoneyTree(uint lastTime, uint freeCount, uint payCount, uint statSum, uint statCount) 
        { 
            this.lastTime = lastTime; 
            this.freeCount = freeCount; 
            this.payCount = payCount; 
            this.statSum = statSum; 
            this.statCount = statCount; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.lastTime)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.freeCount)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.payCount)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.statSum)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.statCount)); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this.lastTime = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.freeCount = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.payCount = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.statSum = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.statCount = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            return pos; 
        } 
    } 
 
    public class SimplePack: Proto4z.IProtoObject //简单示例  
    {     
        //proto id   
        public const ushort protoID = 30005;  
        static public ushort getProtoID() { return 30005; } 
        static public string getProtoName() { return "SimplePack"; } 
        //members   
        public uint id; //id, 对应数据库的结构为自增ID,key  
        public string name; //昵称, 唯一索引  
        public uint createTime; //创建时间, 普通索引  
        public MoneyTree moneyTree;  
        public SimplePack()  
        { 
            id = 0;  
            name = "";  
            createTime = 0;  
            moneyTree = new MoneyTree();  
        } 
        public SimplePack(uint id, string name, uint createTime, MoneyTree moneyTree) 
        { 
            this.id = id; 
            this.name = name; 
            this.createTime = createTime; 
            this.moneyTree = moneyTree; 
        } 
        public System.Collections.Generic.List<byte> __encode() 
        { 
            var data = new System.Collections.Generic.List<byte>(); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.id)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeString(this.name)); 
            data.AddRange(Proto4z.BaseProtoObject.encodeUI32(this.createTime)); 
            if (this.moneyTree == null) this.moneyTree = new MoneyTree(); 
            data.AddRange(this.moneyTree.__encode()); 
            return data; 
        } 
        public int __decode(byte[] binData, ref int pos) 
        { 
            this.id = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.name = Proto4z.BaseProtoObject.decodeString(binData, ref pos); 
            this.createTime = Proto4z.BaseProtoObject.decodeUI32(binData, ref pos); 
            this.moneyTree = new MoneyTree(); 
            this.moneyTree.__decode(binData, ref pos); 
            return pos; 
        } 
    } 
 
} 
 
 
