Welcome to the zsummerX wiki!  
  
# introduction:  
zsummerX 是一款C++ 跨平台的 高性能的 轻量级的网络底层库, 支持TCP, UDP, 拥有完善的定时器机制与日志系统, 并提供一个用于固定包头协议的frame实现, 仅仅通过几行代码就可以实现绝大部分的app,game的网络通讯功能.

zsummerX 相较于较早开源的zsummer来说, 主要区别在于使用C++11标准中的function+bind来重新设计了整个底层库, 无论是库的本身设计还是提供网络接口都变得更为简洁灵活.  

# 优点:    
采用MIT开源许可协议 代码可以免费闭源使用于商业开发.  

纯原生 不依赖任何第三方库, 总体代码量非常精简 没有冗余晦涩设计与书写.  

网络底层使用EPOLL/IOCP实现, 跨LINUX/WINDOWS 32/64平台并且拥有极佳的性能.   

规范通用的proactor接口设计, 可灵活适应绝大多数服务端的使用场景, 包括不仅限于高并发,吞吐,计算,IO,低延迟等. 例如高并发的接入服务, 高吞吐的转发路由服务, 高IO延迟的数据库代理服务, 高计算低延迟的战斗服务, 以及web服务, IM服务等.  
   
完全异步的接口设计.  
  
原始接口文件如下:  
zsummer.h  
tcpaccept.h  
tcpsocket.h  
udpsocket.h  
高级网络接口:(不支持HTTP XML等不定长封包协议)  
FrameHeader.h  
MessageDispatch.h  
TcpSessionManager.h  
  
#编译要求  
windows下VS2013或者以上版本  
linux下g++4.7或者以上版本.  

#compile in linux  
### build   
cd zsummerX/  
cmake -DCMAKE_BUILD_TYPE=DEBUG .   
make  

### runing tcpTest server   
cd zsummerX/example/bin/  
./tcpTest_d 0.0.0.0 81  
### runing tcpTest client   
cd zsummerX/example/bin/   
./tcpTest_d 127.0.0.1 81 1 1  
  
#compile in windows   
###build zsummerX  
cd zsummerX/vc2013/  
compile zsummerX.vcxproj  
### runing tcpTest server   
cd zsummerX/example/bin/  
tcpTest_win32d.exe 0.0.0.0 81  
### runing tcpTest client   
cd zsummerX/example/bin/   
tcpTest_win32d.exe 127.0.0.1 81 1 1  

 


# auther: YaweiZhang  
Web Site: www.zsummer.net  
mail: yawei_zhang@foxmail.com  
github: https://github.com/zsummer
