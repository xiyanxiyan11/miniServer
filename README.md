# Misaka

## 简介
 - 小型的服务器框架，很适合用于嵌入式等环境
 - 连接, 调度, 逻辑层, 业务层分离，利于并行开发
 - 连接方式支持用户扩展
 - 框架本身支持分布式扩展

## DEPEND
 - libev support

## 目录结构
 - tool      调试代码
 - misaksa    程序本体
    - include 头文件代码
    - lib     框架代码
    - link    连接方式横向扩展
    - task    业务层代码
    - src     逻辑注册
 - scripts    辅助脚本

# 组件清单
 - 事物调度
 - 网络层
 - 线程池
 - 调试工具
 - 业务层

## MORE
 - 热备接口支持
 - 调试内存池
 - 性能测试报告
 - 开始编写分布式支持用于实际业务，演示，以及性能测试
 - 充实文档

# 性能测试
 - 测试环境PC<-->虚拟机
 - 客服端X86PC 压测工具smark
 - 服务器, 虚拟机1GRAM,2.4GHZ CPU  
 - 每包512字节,1W个长连接，每秒1W个包
 - cpu消耗 65%~99%
 - 内存开销常驻常驻40M
 - 回复延迟平均900ms

# AUTHOR 
    hongwei mei
    xiyanxiyan10@hotmail.com
