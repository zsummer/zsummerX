Welcome to the zsummerX wiki!  
  
### introduction:  
[![Build Status](https://travis-ci.org/zsummer/zsummerX.svg?branch=master)](https://travis-ci.org/zsummer/zsummerX)  
zsummerX is an net library with cross-platform, high-performance and lightweight, it's depend by C++11 implementation.    
the underlying interface meet the standards proactor, and the implementation used by EPOLL LT / IOCP / SELECT, it's can work in windows / linux / mac  and support TCP, UDP, HTTP.  
the frame interface is advanced and esay, you can use the frame interface to quick development, it's provide OPTIONS module, don't need to worry problem with flexibility.  
    
  
**zsummerX是一个跨win/mac/linux/ios/android平台的高性能轻量级网络库, 采用C++11标准实现.**  
  
  
### interface implementation   
#### 高度灵活的底层接口    
1.     遵从proactor网络模型,与boost asio的接口形式非常接近.
2.     支持TCP, UDP, 支持多线程.  
3.     支持post接口,可以将任意外部线程的数据和方法通过该接口转移到zsummer的loop中执行.  
4.     支持定时器.  
5.     需要使用者维护缓存的生命周期管理, 需要使用者对底层有所了解.  
   
#### 更易用但仍然拥有足够灵活性的frame接口   
1.      在底层Socket抽象之上再度封装出Session抽象, 并由SessionMansger单例来统一管理.  
2.      封装掉除了socket建立,断开,消息处理外的所有实现, 然后通过OPTION接口来给需要一定灵活性的需求以简洁干净的定制切口.    
3.      多线程方案因单例影响, 只能在io处理后进行多线程方案, socket io只能单线程. (游戏行业和大多数行业即使用环境都足够用了, 不够的话可以多进程方案)    
  
   
#### frame接口提供以下特性支持    
1.      支持[proto4z](https://github.com/zsummer/proto4z)的二进制协议(支持C++,lua,C#,SQL).  
2.      支持http协议, 可以几行代码构建出一个web服务器或者web客户端.    
3.      以上两种协议为Option中的默认定制方案,通过定制option的默认接口, 可以支持任意二进制协议.        
4.      支持socket断线重连, 断线重连参数可以通过Option定制.      
5.      支持socket属性修改,例如NoDelay等属性.  
6.      session提供定时器消息, 注册该消息后每个session在间隔一定时间后就会收到一次消息, 方便进行例如心跳检测的实现.  
7.      支持UserParam, 通过让session携带自定义数据,从而可以做到获取一些属性时的O(1)级的性能,而避免在业务层进行map做额外索引.  例如针对Session的socket建立时间的记录, 上一次心跳检测到的记录, 该session是否有进行登录认证,以及相关的记录, 这个特性可以简化上层逻辑的实现并提高性能, 而实现只是几行代码的代价.   
8.      支持RC4流加密, RC4的加密开启与否, 密钥等都可以通过option进行定制,可以有效防止重放攻击和旁路监听等.     
9.      tcp支持flash的policy策略认证.  
10.      支持限制accept的最大连接数, 超出该上限则新accept的socket会立刻踢下去.  
11.     完善的异常保护机制,在try catch种执行使用者提供的消息处理回调方法,可以做到捕获所有C++的异常从而把异常错误限制在单个消息处理流程中.
12.     提供运行时的堆栈记录接口, 在抛出异常的时候可以把堆栈信息打印到日志.  
13.     proto4z的异常均会携带堆栈信息,方便判断错误.
14.     强大的[log4z日志系统](https://github.com/zsummer/log4z), zsummerX的网络库日志单独使用一个logger,并可以独立控制该logger的日志级别,显示与否等相关控制.  
15.     提供并包优化方案, 可以一次性最大可能读取和写入网络数据.在洪水测试和实际项目中,该机制的性能提升非常明显.  
16.     最大发送队列控制, 可以防止因网络异常或者非法攻击造成的内存失控.  
17.     可以手动控制zsummerX的网络缓冲的创建和销毁, 以提高内存使用的性能.  
18.     完备的统计信息,包括底层io次数, 吞吐量, 数据包发送和接受次数, 发送队列长度, 限制free内存数量和使用率等统计信息.  
19.     提供友好简洁的网络关停方案.  
20.     一键编译, 不依赖其他任何需要预先编译的依赖库, down下代码直接在任意平台上可以编译出静态库以及测试文件.  
21.     LoopEvent支持立即返回模式和无事件等待模式, 前者可以寄生在类似U3D的update中, 方便嵌入客户端.  
22.     简洁务实的实现方案, 代码量小于6000行.  
23.     对C++11的shared_ptr和functional的有效使用, 接口的易用性和上层代码的稳定性接近脚本语言的水平.   
24.     提供[lua胶水代码 summer.h summer.cpp](https://github.com/zsummer/zsummerX/tree/master/example/luaTest), 可以直接在lua中使用zsummerX.    
  
#### frame options 代码预览   
```C++

        enum ProtoType
        {
            PT_TCP,
            PT_HTTP,
        };
        
        struct SessionOptions 
        {

            ProtoType       _protoType = PT_TCP;
            std::string     _rc4TcpEncryption = ""; //empty is not encryption 
            bool            _openFlashPolicy = false; //check falsh client  
            bool            _setNoDelay = true; 
            bool            _joinSmallBlock = true; //merge small block  
            unsigned int    _sessionPulseInterval = 30000;  
            unsigned int    _connectPulseInterval = 5000;  
            unsigned int    _reconnects = 0; // can reconnect count 
            bool            _reconnectClean = true;//clean unsend block . 
            unsigned int    _maxSendListCount = 600;
            OnBlockCheck _onBlockCheck;
            OnBlockDispatch _onBlockDispatch;
            OnHTTPBlockCheck _onHTTPBlockCheck;
            OnHTTPBlockDispatch _onHTTPBlockDispatch;
            OnSessionEvent _onSessionClosed;
            OnSessionEvent _onSessionLinked;
            OnSessionEvent _onSessionPulse;

            CreateBlock _createBlock ;
            FreeBlock _freeBlock;
        };
        
        struct AccepterOptions
        {
            AccepterID _aID = InvalidAccepterID;
            TcpAcceptPtr _accepter;
            std::string        _listenIP;
            unsigned short    _listenPort = 0;
            unsigned int    _maxSessions = 5000;
            std::vector<std::string> _whitelistIP;
            unsigned long long _totalAcceptCount = 0;
            unsigned long long _currentLinked = 0;
            bool _closed = false;
            SessionOptions _sessionOptions;
        };
```   
### compile conditions   
on windows need VS2013 or latter version  
on linux need g++ 4.7 or latter version  
  
### how to compile on linux, mac    
cd zsummerX/  
cmake .  
make  

### how to compile on windows   
open zsummerX.sln and compile  

### runing frameStressTest  
**server**  
cd zsummerX/example/bin/  
./frameStressTest_d 0.0.0.0 81  

**client**  
cd zsummerX/example/bin/   
./frameStressTest_d 127.0.0.1 81 1  

  
### About The Author  
**Author**: YaweiZhang  
**mail**: yawei.zhang@foxmail.com  
**github**: https://github.com/zsummer/zsummerX  
