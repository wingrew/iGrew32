# 类MIPS32--iGrew32 CPU文档


## 一、项目概述
本项目旨在使用c++语言编写一个单周期和多周期的类MIPS32的cpu模拟器，此外添加了五级流水线的时序模拟功能。

项目目录树如下：
```
iGrew32
├── one_cycle         // 单周期      
│   ├── alu.h        // ALU运算器件        
│   ├── cpu.cpp       // cpu主要运行代码         
│   ├── fast.h      // 常量定义与指令设计
│   ├── mem.h       // 存储器与寄存器
├── multi_cycle       // 多周期        
│   ├── alu.h                
│   ├── cpu.cpp                
│   ├── fast.h 
│   ├── mem.h                  
├── expand            //汇编转译   
│   ├── const.py        //指令设计   
│   ├── transfer.py       //汇编转译主要程序
│   ├── instruct        // 二进制机器码
│   ├── instruct.mem    // MIPS汇编代码
├── five_stage            //五级流水线时序模拟
```

## 二、编码规则
这次实验指令编码设计主要参考MIPS，格式极其相似但是具体编码不同，目前实现了以下指令：
| R类型  | I类型  | J类型  |  F类型  |
| :----: | :----:| :----: | :----: |
| add    | addi | j   |  addf  |
| sub    | muli | jal |  subf  |
| mul    | slti |     |  mulf  |
| slt    | andi |     |  divf  |
| and    | ori  |     |  lwf   |
| or     | xori |     |  swf   |
| xor    | lw   |     |        |
| nor    | sw   |     |        |
|sllv    | beq  |     |        |
|srlv    | bne  |     |        |
|srav    |      |     |        |
|jr      |      |     |        |
|div     |      |     |        |
| sll    |      |     |        |
|srl     |      |     |        |
| sra    |      |     |        |
|jalr    |      |     |        |

总计35条指令，已经能够支持一般程序运行。
编码规则如下：
![这是图片](./image/image1.png/ "R类型")
![这是图片](./image/image2.png/ "I类型")
![这是图片](./image/image3.png/ "F类型")
![这是图片](./image/image4.png/ "J类型")

## 三、性能评价
依据DDCA一书7.2例题设定单周期时间和多周期时间。
cycle_once = 925ps
cycle_multi = 325ps

| CPU类型  | 平均CPI  | 总周期数 | 总指令数 |  模拟计算时间  |
| :----: | :----:| :----: | :----: |:----: |
| 单周期 | 1   |  453  |  453   |419025.00ps|
| 多周期 | 4.0 |  1816 |  453   |590200.00ps|

## 四、流水线设计

本次采取时序模拟的方法对流水线进行模拟。以生产者-消费者模型为基础，生成两个线程。一个线程产生指令流并执行指令，另一个线程则进行时序模拟。对于给定程序，阻塞发生在swf后mulf和bne，通过output.txt文件，你可以很清楚地看到这一现象。