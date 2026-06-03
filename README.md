# Jungle

Jungle 是一个实验性游戏引擎，旨在探索现代 C++ 标准（C++26）在游戏开发中的应用。项目充分利用编译期反射、契约（contracts）等新特性，构建类型安全、零开销的基础设施层。

### 模块结构

```
jungle-base/     基础库：类型系统、序列化框架、反射工具、测试框架
jungle-core/     ECS 核心：Entity、Component、Archetype
jungle-api/      占位模块
jungle-client/   占位模块
jungle-server/   占位模块
jungle-ui/       占位模块
```

当前有实际代码的模块是 `jungle-base` 和 `jungle-core`，其余为预留模块。

## 构建

### 依赖

- CMake 3.20+
- 支持 C++26 反射扩展（`-freflection`）的 GCC 编译器
- Ninja（推荐构建系统）

### 配置与编译

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

编译器选项：C++26 标准、反射扩展（`-freflection`）、契约（`-fcontracts`）、禁用 RTTI 和异常。

## 运行测试

```bash
ctest --test-dir build --output-on-failure
```

单元测试位于 `jungle-base/unit_tests/`，集成测试位于 `tests/`。

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
