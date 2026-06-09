# Jungle

Jungle 是一个实验性游戏引擎，旨在探索现代 C++ 标准（C++26）在游戏开发中的应用。项目充分利用编译期反射、契约（contracts）等新特性，构建类型安全、零开销的基础设施层。

## 文档

- [简体中文](https://pointertobios.github.io/Jungle/zhcn/)

## Roadmap

- [x] 基础类型系统（固定宽度整数、Unicode 字符串）
- [x] 编译期反射元编程工具
- [x] 序列化/反序列化框架（serde）
- [x] 注解驱动的字段控制（`[[=customized]]`、`[[=field]]`、`[[=customize<C>]]`）
- [x] ECS 核心基元（Entity、Component、Archetype、Manager）
- [x] 测试框架（JUNGLE_SYNC_TEST）
- [x] 基于 mdBook 的文档系统（中/英双语）
- [ ] ECS 查询系统（Query、System）
- [ ] 资源管理（Asset pipeline）
- [ ] 渲染抽象层
- [ ] 网络层

## License

本项目使用 **MIT License** 发布.

Copyright (C) 2025-Present pointer-to-bios <pointer-to-bios@outlook.com>

完整的法律条款参见 [LICENSE](./LICENSE.md)。
