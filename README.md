Welcome to the ZSUMMER_11X wiki!  
  
# introduction:  
zsummer_11x是一款C++ 跨平台的 高性能的 轻量级的网络底层库, 支持TCP, UDP, 拥有完善的定时器机制与日志系统.    
zsummer_11x相较于zsummer来说, 使用function+bind来代替原先的抽象类接口, 让库的使用更简洁灵活. 使用shared_ptr来控制对象的生命周期让代码在流程上更为简约.  

# 优点:    
MIT开源 代码可以任意使用在任何场合.  
纯原生 不依赖任何第三方库.  
使用EPOLL/IOCP纯手工实现,采用优雅简洁的设计, 跨LINUX/WINDOWS 32/64平台并且拥有极佳的性能.   
规范的proactor一致的接口设计, 灵活适应绝大多数服务端的使用场景, 包括不仅限于高并发,高吞吐情境, 游戏后端,游戏前端, web Server, IM server等.   
每个IOSERVER都可以无限制挂靠的connecter角色, accepter角色与clienter角色 充分应对各种复杂的网络使用场景并充分发挥多核性能.    
完全异步的接口设计.  
  

#编译需求  
windows下VS2012或者以上版本  
linux下g++4.7或者以上版本.  

#编译方法  
# build server & client  
in linux:   
cd zsummer_11x/example  
cmake .  
make  

in windows:  
cd zsummer_11x/example  
use vs2012 open  zsummer_11x.sln  


# auther: 张亚伟 YaweiZhang  
Web Site: www.zsummer.net  
mail: yawei_zhang@foxmail.com  
github: https://github.com/zsummer
