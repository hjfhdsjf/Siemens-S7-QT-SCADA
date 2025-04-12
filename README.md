<!--
 * @Author: Magic
 * @Date: 2025-04-11 21:41:40
 * @LastEditTime: 2025-04-12 09:58:07
 * @node: 
-->
# S7助手_V1.0.1

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

一个基于Qt和Snap7库开发的西门子PLC调试工具，支持S7-1200/1500系列PLC的int、string、bool、char、
real数据读写，DB、I、M、Q四个区域选择，以及创建循环任务定时监控变量。

---

## 功能特性

- 🎯 **多数据类型支持**  
  支持STRING、INT、BOOL、CHAR、FLOAT类型数据读写
- 🗂️ **存储区选择**  
  支持DB/I/Q/M存储区操作，DB区需指定DB号
- 📊 **日志系统**  
  双日志窗口设计（信息日志+任务日志），支持彩色状态提示
- 🔄 **循环任务**  
  可配置并行循环任务，自定义区域/数据类型/采集间隔
- 🧵 **多线程架构**  
  采用Worker-Thread模式实现非阻塞读写操作

**环境要求**
   - Qt 5.15+ 
   - Snap7库(自带)
   - MSVC2019 64bit

**版本说明**
V1.0
实现基础功能，读写 string、real、bool、int、char类型读写
循环任务创建定时读取
数据区域选择DB、I、M、Q四个区域选择
日志输出不同颜色

V1.0.1
细节修改，增加断开连接后，自动清除所有任务

**后续支持**
   - 其它品牌plc读写
   - s7_smart200PLC各种数据类型读写
   - s7_300PLC各种数据类型读写
   - mqtt转发功能
   - modbustcp读写功能
