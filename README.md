Welcome to the zsummerX wiki!  
  
# introduction:  
[![Build Status](https://travis-ci.org/zsummer/zsummerX.svg?branch=master)](https://travis-ci.org/zsummer/zsummerX)  
zsummerX is an C++ cross-platform, high-performance and lightweight net library.  

zsummerX implementation used by EPOLL LT / IOCP / SELECT,  the interface use proactor.  
zsummerX can work in windows / linux / mac / android / iOS .  

zsummerX support TCP, UDP, HTTP , and provide a easy frame interface that you can use a little code to implement a strong server .  

zsummerX depend by  C++ 11  implementation   
  
# traits :    
used MIT license , free and business friendly.  

needless third-party library, simple and strong .  

high-performance and high-concurrency on linux or windows.  

can work on meny operattion if used select . example mac , iOS , android . 


#compile conditions   
on windows need VS2013 or latter version  
on linux need g++ 4.7 or latter version  
  
#how to compile on linux  
###build zsummerX  
cd zsummerX/  
cmake . -DCMAKE_BUILD_TYPE=DEBUG   
make  

### runing frameStressTest server   
cd zsummerX/example/bin/  
./frameStressTest_d 0.0.0.0 81  
### runing frameStressTest client   
cd zsummerX/example/bin/   
./frameStressTest_d 127.0.0.1 81 1  
  
#how to compile on windows   
###build zsummerX  
cd zsummerX/vc2013/  
open compile zsummerX.vcxproj  
### runing frameStressTest server   
cd zsummerX/example/bin/  
start frameStressTest_win32d.exe 0.0.0.0 81  
### runing frameStressTest client   
cd zsummerX/example/bin/   
start frameStressTest_win32d.exe 127.0.0.1 81 1  

#compile in mac iOS  
###build zsummerX  
cd zsummerX/  
cmake . -DCMAKE_BUILD_TYPE=DEBUG  
make  

### runing frameStressTest server   
cd zsummerX/example/bin/  
./frameStressTest_select_d 0.0.0.0 81  
### runing frameStressTest client   
cd zsummerX/example/bin/   
./frameStressTest_select_d 127.0.0.1 81 1  
  
#compile in android or other  
### build   
cd zsummerX/  
cmake . -DZSUMMER_USE_SELECT -DCMAKE_BUILD_TYPE=DEBUG  
make  

### runing frameStressTest server   
cd zsummerX/example/bin/  
./frameStressTest_select_d 0.0.0.0 81  
### runing frameStressTest client   
cd zsummerX/example/bin/   
./frameStressTest_select_d 127.0.0.1 81 1  
  
# Author: YaweiZhang  
Web Site: www.zsummer.net  
mail: yawei_zhang@foxmail.com  
github: https://github.com/zsummer  
