<!--
 * @Author: Magic
 * @Date: 2025-04-11 21:41:40
 * @LastEditTime: 2025-04-11 22:05:14
 * @node: 
-->
# S7助手_V1.0

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

一个基于Qt和Snap7库开发的西门子PLC调试工具，支持S7-1200/1500系列PLC的数据读写与循环任务监控。

---

## 功能特性 | Features

### 中文
- 🎯 **多数据类型支持**  
  支持STRING、INT、BOOL、CHAR、FLOAT类型数据读写
- ⚙️ **PLC参数配置**  
  可设置IP地址、机架号(Rack)、插槽号(Slot)
- 🗂️ **存储区选择**  
  支持DB/I/Q/M存储区操作，DB区需指定DB号
- 📊 **日志系统**  
  双日志窗口设计（信息日志+任务日志），支持彩色状态提示
- 🔄 **循环任务**  
  可配置10个并行循环任务，自定义区域/数据类型/采集间隔
- 🧵 **多线程架构**  
  采用Worker-Thread模式实现非阻塞读写操作

**环境要求**
   - Qt 5.15+ 
   - Snap7库(自带)
   - MSVC2019 64bit

**后续支持**
   - 其它品牌plc读写
   - s7_smart200PLC各种数据类型读写
   - s7_300PLC各种数据类型读写
   - mqtt转发功能
   - modbustcp读写功能
