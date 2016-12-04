# Misaka

## 简介
 - 服务器框架，很适合用于嵌入式等环境
 - 连接, 调度, 逻辑层, 业务层分离，利于并行开发
 - 支持热更新，动态加载
 - 连接方式支持用户扩展
 - 框架持分布式扩展

## DEPEND
 - libev
 - zlog     
 - lua-dev
 - python-dev

## 目录结构
 - tool             调试代码
 - scripts          辅助脚本
 - misaka           程序本体
    - include       头文件
    - lib           框架代码
    - link          通信插件
    - task          业务层代码(c)
    - task-lua      业务层代码(lua)
    - task-python   业务层代码(python)
    - app           应用模块
    - parse         协议解析接口

# 组件清单
 - 调度
 - 业务分发
 - 网络层
 - 线程池
 - 调试工具
 - 动态加载

## MORE
 - 暂时无法使用了，亲。施工中，主要追加几个统一的包解析方式以及脚本化的彻底封装
 - 打算4层次的parser协议主要使用一个protobuf的代码进行模块功能展示意
 - 尝试绑定一个核专门用于对外转发
 - python脚本支持
 - system管理接口(远程管理,定时器统一管理)
 - web 对接
 - 性能测试报告
 - 充实文档
 - 内存池support
 - 演示代码

# AUTHOR 
    - name: hongwei mei
    - email:xiyanxiyan10@hotmail.com
    - weixin: xiyanxiyan10
