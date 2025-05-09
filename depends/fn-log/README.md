
# Introduction:  
[![Build](https://github.com/zsummer/fn-log/actions/workflows/cmake.yml/badge.svg)](https://github.com/zsummer/fn-log/actions/workflows/cmake.yml)
[![CodeQL](https://github.com/zsummer/fn-log/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/zsummer/fn-log/actions/workflows/codeql-analysis.yml)
[![Coverity Status](https://scan.coverity.com/projects/19065/badge.svg)](https://scan.coverity.com/projects/zsummer-fn-log)
[![Coverage Status](https://coveralls.io/repos/github/zsummer/fn-log/badge.svg?branch=master)](https://coveralls.io/github/zsummer/fn-log?branch=master)  
fn-log is an open source C++ lightweight & cross platform log library. It's an iteration from log4z.   
It provides in a C++ application log and trace debug function for 7\*24h service program.    
Support 64/32 of windows/linux/mac/android/iOS.    
fn-log是一款开源的轻量级高性能的跨平台日志库, 从log4z迭代而来, 主要针对7\*24小时服务器程序的日志输出与跟踪调试,   
支持64/32位的windows/linux/mac/android/iOS等操作系统.   

# Feature:  

- [x] **MIT开源授权 授权的限制非常小.**   

- [x] **跨平台支持linux & windows & mac, 仅头文件实现.**   
  > 通过merge_header.sh脚本可以合并为单个头文件实现.  
  
- [x] **生命周期自动化管理, 接口简洁易用.**    

- [x] **高性能的线程安全实现.**   
  > std::atomic实现的lockfree方案, 跨平台并且极低的竞争代价, 多线程下有比单线程更好的性能表现(mutex实现多线程下性能会大幅损失)   
  > 文件写入可以达到300万行以上/秒, 不执行文件写入动作则可达到800万行/秒(完整的序列化和通道队列和处理流程).   
  
- [x] **高性能的流程设计和高性能的序列化实现.**   
  > 基础类型通过lut表进行序列化, 流程热点均有压测调优  
  > 文件写入可以达到300万行以上/秒, 不执行flush刷入动作(完整的序列化和通道队列)可达到800万行/秒.   
  
- [x] **完备的线程安全实现 所有入口流程都做出了线程安全保证**   
  > 多线程下任意线程开始构造和启停, 任意线程读写日志, 任意线程修改CFG配置(原子动作)    

- **基于SHARED MEMORY的宕机恢复方案** 参见测试用例test_shm.cpp  
  > 宕机后fn-log会自动使用shm上的数据进行重建和继续完成未写入的日志.  
  
- [x] **兼容yaml的配置解析器 支持热更新**      
  > 支持延迟热更新和实时热更新  
  
- [x] **CHANNEL-DEVICE M:N组合方式的多管道-多输出端设计.**    
  > 简洁的形式提供极强的扩展性, 可以满足大型项目的多种交叉输出和分流输出的复杂需求.  
  > 可以针对不同的channel以及不同的device进行独立设置等级/内容过滤机制等.  
  > 提供各种形式的device输出机制, 已实现的包括写文件, 写屏幕, 写UDP等.   
  > 可以同时支持同步输出和异步输出  
  > 支持虚拟设备 支持用户进行特殊导出等处理, 并保存有日志现场信息.  
  
- [x] **常规日志功能支持** 
  > 支持日志文件回滚  
  > 支持日期文件夹滚动  
  > 支持屏显日志染色输出  
  > 支持LOG4Z风格的流式输入  
  > 支持C++ stream风格的流式输入  
  > 支持模版参数包形式的类型安全的动参输入  
  > 支持C风格的FORMAT输入  


#  Example  

### Log Format   
``` c
[20190514 16:47:20.536][ALARM] [15868]channel [0] start.

[20190514 16:47:20.548][DEBUG] [15868] fn-log\tests\simple_test.cpp:<40> main log init success
[20190514 16:47:20.548][DEBUG] [15868] fn-log\tests\simple_test.cpp:<42> main now time:1557823640;
[20190514 16:47:20.548][DEBUG] [15868] fn-log\tests\simple_test.cpp:<44> main hex text:
	[
	[0x7FF67B00B4B4:  f  n  l  o  g  . 
	[0x7FF67B00B4B4: 66 6E 6C 6F 67 00 
	]
	
[20190514 16:47:20.548][ALARM] [15868] fn-log\tests\simple_test.cpp:<46> main finish
```

### Fast Use Example With Out Yaml File    
``` cpp
#include "fn_log.h"

int main(int argc, char* argv[])
{
    int ret = FNLog::FastStartDefaultLogger();
    if (ret != 0)
    {
        return ret;
    }

    LogAlarm() << "log init success";

    LogDebug() << "now time:" << time(nullptr) << ";";
    LOGD("now time:" << time(nullptr) << ";");
    LogDebugPack(0, 0, "now time:", time(nullptr), ";");
    
    LogAlarm() << "finish";

    return 0;
}
```

### Fast Use Example With Yaml File    
yaml file    
``` yaml
# 配表文件  

 hot_update: true
# 0通道为多线程带文件和屏显输出
 - channel: 0  
    priority: trace
    category: 0
    category_extend: 0
    -device: 0
        disable: false
        out_type: file
        priority: trace
        category: 0
        category_extend: 0
        path: "./log/"
        file: "$PNAME_$YEAR$MON$DAY"
        rollback: 4
        limit_size: 1000 m #only support M byte
    -device: 1
        disable: false
        out_type: screen
        category: 0
        category_extend: 0
 # 1通道为多线程不挂任何输出端
 - channel: 1

 # 2通道为单线程同步写文件
 - channel: 2
    sync_write: 1 #only support single thread
    -device: 0
        disable: false
        out_type: file

 # 3通道为单线程无输出端
 - channel: 3
    sync_write: 1 #only support single thread


```
code   
```  cpp 
#include "fn_log.h"

int main(int argc, char* argv[])
{
    int ret = FNLog::LoadAndStartDefaultLogger("./log.yaml");
    if (ret != 0)
    {
        return ret;
    }
    
    int limit_second = 50;
    while (limit_second > 0)
    {
        LogDebug() << "default channel.";
        LogDebugStream(1, 0) << "channel:1, category:0.";
        LogDebugStream(2, 0) << "channel:2, category:0.";
        LogDebugStream(3, 0) << "channel:3, category:0.";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        limit_second--;
    }


    LogAlarm() << "finish";
    return 0;
}

```  


# How To Use  
### multi header file  
cp src/include/\*.h to dst project.   
### single header file  
cp fn_log.h.only to dst project and remove suffix ".only"   
> the file fn_log.h.only merge from src/include/\*.h  

### Supported Compilers  
* GCC >= 4.8  
* MSVC >= VS2015  
* CLANG >= 3.3  

# How To Test  
``` shell  
mkdir build
cd build
cmake ..
make
cd ../bin
./buffer_test
./buffer_correct_test
./load_config_test
./simple_test
./stress_udp_test
./stress_test
./multi-thread_test
./multi-thread_write_file_test
```


# Yaml Manual  
### Global Option:  
- [x] hot_update
  > option: true false  
  > default: false  
  > desc: moniter yaml file modify and update logger option.  

- [x] shm_key 
  > option: hex number  
  > default: 0  
  > desc: fn-log will used the number to create at shm memory , if already has shm memory will load it and run continue.    

- [x] logger_name 
  > option:    
  > default:  
  > desc: logger name    

- [x] logger_desc 
  > option:   
  > default:  
  > desc: logger desc   

- [x] define  
  > option: list format like "key:val, key:val, key:val"
  > default:
  > desc: like c/c++ #define;  to replace the follow config content. warn: val length need equal or less than key length(maybe change impl).  

### Channel Option: (channel.)   
- [x] sync
  > option: async sync 
  > default: async  
  > desc: async mod will open thread to async write log  
  > desc: sync mod all operator in master thread  

- [x] priority  
  > option: trace debug info warn error alarm fatal   
  > default: trace  
  > desc: log will discard when log priority less than filter priority.  

- [x] category  
- [x] category_extend
  > option:  
  > default: 0, invalid value.  
  > desc: log will reserve when category in set [category, category+category_extend), and other not.   
  > desc: mark content category to filter or write diff device

- [x] category_wmask
  > option: number  
  > defualt: 
  > desc: white list for category, a bitmap by u64.  
- [x] category_bmask
  > option: number  
  > defualt: 
  > desc: sugar for 'category_mask'  black list; 

- [x] category_wlist
  > option:   list format "1,23,44,1"   
  > defualt: 
  > desc: sugar for 'category_mask'  white list;        
- [x] category_blist
  > option:   
  > defualt: 
  > desc: sugar for 'category_mask'  black list;      
   
- [x] identify_wmask
  > option: number  
  > defualt: 
  > desc: white list for identify, a bitmap by u64.  
- [x] identify_bmask
  > option: number  
  > defualt: 
  > desc: sugar for 'identify_mask'  black list; 
- [x] identify_wlist
  > option:   list format "1,23,44,1"   
  > defualt: 
  > desc: sugar for 'identify_mask'  white list;        
- [x] identify_blist
  > option:   
  > defualt: 
  > desc: sugar for 'identify_mask'  black list;        

### Device Option: (channel.device.)  
- [x] disable  
  > option: true, false  
  > default: true  
  > desc: the device will ignore in proc log when this option is disable state.  

- [x] out_type  
  > option: null, file, udp, screen, virtual   
  > default: null  
  > desc: as the option name.   only one virtual device per one channel.  

- [x] priority  
  > option: trace debug info warn error alarm fatal   
  > default: trace  
  > desc: log will not process when log priority less than filter priority.  

- [x] category
- [x] category_extend
  > option:  
  > default: 0, invalid value.  
  > desc: log will process when category in set [category, category+category_extend), and other not.   

- [x] category_wmask
  > option: number  
  > defualt: 
  > desc: white list for category, a bitmap by u64.  
- [x] category_bmask
  > option: number  
  > defualt: 
  > desc: sugar for 'category_mask'  black list; 
- [x] category_wlist
  > option:   list format "1,23,44,1"   
  > defualt: 
  > desc: sugar for 'category_mask'  white list;        
- [x] category_blist
  > option:   
  > defualt: 
  > desc: sugar for 'category_mask'  black list;        

- [x] identify
- [x] identify_extend
  > option:  
  > default: 0, invalid value.  
  > desc: log will process when identify in set [identify, identify+identify_extend), and other not.   

- [x] identify_wmask
  > option: number  
  > defualt: 
  > desc: white list for identify, a bitmap by u64.  
- [x] identify_bmask
  > option: number  
  > defualt: 
  > desc: sugar for 'identify_mask'  black list; 
- [x] identify_wlist
  > option:   list format "1,23,44,1"   
  > defualt: 
  > desc: sugar for 'identify_mask'  white list;        
- [x] identify_blist
  > option:   
  > defualt: 
  > desc: sugar for 'identify_mask'  black list;        


- [x] udp_addr  
  > option:  
  > default:  
  > desc: in out_type:udp valid.  
  > desc: example format 127.0.0.1:8080   

- [x] path  
  > option:  
  > default: "./"  
  > desc: in out_type:file valid.  
  > desc: out file path.  

- [x] file 
  > option:   
  > ```default: "$PNAME_$YEAR$MON$DAY_$PID."```  
  > desc: in out_type:file valid.  
  > desc: diy out file name. support escape string: $PNAME $PID $YEAR $MON $DAY $HOUR $MIN $SEC  

- [x] rollback
  > option: 
  > default: 0  
  > desc: in out_type:file valid.  
  > desc: 0 is no rollback op, and other number is rollback file count.   

- [x] limit_size
  > option: 
  > default: 0  
  > desc: in out_type:file valid.  
  > desc: 0 is no limit, and other number is rollback file limit size (M byte).   

- [x] stuff_up
  > option: 
  > default: false  
  > desc: in out_type:file valid.  
  > desc: rollback files when they are out limit size.   
  


 # default_log.yaml  
  ```yaml

# 基础语法   
## 缩进 相同scope的field需要保持缩进一致
## 缩进 子层级field缩进大于父层级field  

## 数组 以-为前缀 需要放在key之前   
## 注释 任意行内读到#之后会停止该行的解析操作, 即使位于引号内.  
## kv分隔符 : 
## 空白符 通常读到空白符会自动跳过, 但不包括尾随在值中间的空白符  
## key 必须为小写字母和下划线 不允许其他值  
## 值 会自动裁切前缀空白  
## 值 的行尾边界有["#\r\n\0]
## 值 的内容做了约束 范围为: 字符数字[a-zA-Z0-9]  [_-:/.,$~%]   
## 值 的内容需求上为 文件名和路径 数字 字符 因此通过约束可以减少不必要的配置问题  

# 错误码  代码中搜索ParseErrorCode, ErrNo查找对应说明. 

# 宏语义 当前实现方式 原地替换(inplace)**随后**的所有字符串内容  
# 宏语义 要注意替换后的内容不能长于原有内容, 可以避免内存重分配开销.
# 宏语义 要注意不要替换掉正常的字符串, 包含值中的内容  
 

# 变量 和宏替换类似, 不同点在于执行替换动作时候会增加前缀$进行替换, 并包裹{}重复执行一次.   例如var:{tag0=1}  会替换掉${tag0} 和 $tag0  

# 宏,变量 均不能重复定义 (替换无法再替换)   
# 宏,变量 通常为符号名+字面量 用来定义在随后重复使用的数字类编号 category, indentify, mask等  


- define: LIST______ 0,1,2,3   # used like c-style;  warn: def name len must bigger than content.    
- def: EMPTY______    

- var: id = 234
- var: id0 = 0, id1 =1    # use var $id0  or ${id0}  
- var: {key="sss", val=999}   


# - shm_key: 33321231
# - hot_update:[true][false]
# - logger_name: string  # log content prefix "LOG_PREFIX_NAME" 
# - logger_desc: string  # log content prefix "LOG_PREFIX_DESC" 
 - channel: 0
    sync: async  
    #sync: [async][sync][ASYNC][SYNC]
    #priority:[trace][debug][info][warn][error][alarm][fatal]
    #category: id
    #category_extend: count
    #category_wmask: 255 (white mask)
    #category_wlist: {2,3,43} (white list)
    #category_bmask: 255 (black mask)
    #category_blist: {1,23,4} (black list)
    #identify: id
    #identify_extend: count
    #identify_wmask: 255 (white mask)
    #identify_wlist: {2,3,43} (white list)
    #identify_bmask: 255 (black mask)
    #identify_blist: {1,23,4} (black list)
    - device: 0
        disable: false
        #disable: [false][true][FALSE][TRUE]
        #priority:[trace][debug][info][warn][error][alarm][fatal]
        #category: id
        #category_extend: count
        #category_wmask: 255 (white mask)
        #category_wlist: {2,3,43} (white list)
        #category_bmask: 255 (black mask)
        #category_blist: {1,23,4} (black list)
        #identify: id
        #identify_extend: count
        #identify_wmask: 255 (white mask)
        #identify_wlist: {2,3,43} (white list)
        #identify_bmask: 255 (black mask)
        #identify_blist: {1,23,4} (black list)
        out_type: file
        #out_type:[file][null][udp][screen][virtual]
        file: "$PNAME"  #in file type it's log file name
        path: ./ #in file type it's log file path  
        rollback: 4 #in file type it's this device log file rollback count.   
        limit_size: 100 m #in file type it's one log file limit size  
        #stuff_up: [true][false]  #in file type it's 'false' will rollback old file when reopen logger.  the 'true' will append exist log file still reach 'limit_size'  
        #udp_addr: [ip:port]  # in udp type     
    - device: 1
        disable: false
        out_type: file
        priority: error
        file: "$PNAME_error"
        rollback: 4
        limit_size: 100 m #only support M byte
    - device: 2
        disable: false
        out_type: screen
        priority: info
 - channel: 1
    sync: sync
    - device: 0
        disable: true

  ```


# About The Author  
Author: YaweiZhang  
Mail: yawei.zhang@foxmail.com  
QQGROUP: 524700770  
GitHub: https://github.com/zsummer/fn-log  

