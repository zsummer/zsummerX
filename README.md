Welcome to the zsummerX wiki!  
  
### introduction:  
[![Build Status](https://travis-ci.org/zsummer/zsummerX.svg?branch=master)](https://travis-ci.org/zsummer/zsummerX)  
zsummerX is an C++ cross-platform, high-performance and lightweight net library.  
the interface use proactor, the implementation used by EPOLL LT / IOCP / SELECT, it's can work in windows / linux / mac .  
it's support TCP, UDP, HTTP , and provide a easy frame interface that you can use a little code to implement/join one strong server .  
zsummerX depend by  C++ 11  implementation   

zsummerX是C++11实现的proactor模型的网络库, 实现分底层,frame层.  
底层是封装不同平台差异化的接口并提供一个统一的proactor接口, 使用epoll,iocp,select实现, 其中select兼容linux和mac系统以及其移动平台的adroid,iOS系统, 支持tcp,udp协议.    
frame层提供高级的封装, 通过勾选option选项,可以很方便的实现http协议处理(简单的web服务器或者客户端), 自定义的二进制协议(默认的二进制tcp协议是proto4z实现的), 并支持短线重连, rc4连接加密, 支持flash的Policy策略认证, 支持最大连接数,最大消息队列等.


  


###compile conditions   
on windows need VS2013 or latter version  
on linux need g++ 4.7 or latter version  
  
###how to compile on linux, mac    
cd zsummerX/  
cmake .  
make  

###how to compile on windows   
open zsummerX.sln and compile  

### runing frameStressTest  
**server**  
cd zsummerX/example/bin/  
./frameStressTest_d 0.0.0.0 81  

**client**  
cd zsummerX/example/bin/   
./frameStressTest_d 127.0.0.1 81 1  

  
### Author: YaweiZhang  
Web Site: www.zsummer.net  
mail: yawei.zhang@foxmail.com  
github: https://github.com/zsummer  
