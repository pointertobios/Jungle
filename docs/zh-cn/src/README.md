# Jungle 引擎

Jungle 是一个基于 C++26 反射的实验性游戏引擎，探索现代 C++ 标准在游戏开发中的应用。

## 设计理念

- **零开销抽象**：编译期反射和模板元编程替代运行时类型系统，不依赖 RTTI 和异常
- **类型安全**：通过 concept、contract 和强类型别名在编译期捕获错误
- **模块化**：基础库、ECS 核心、渲染、网络等按依赖分层

## 编译要求

- 暂时仅支持 GCC
- C++26 标准
- 反射扩展（`-freflection`）
- 契约（`-fcontracts`）
- 禁用 RTTI（`-fno-rtti`）
- 禁用异常（`-fno-exceptions`）
