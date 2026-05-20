# Jungle 代理指引

## 项目概况

- 这个仓库是一个根级 CMake 工程，由 [CMakeLists.txt](CMakeLists.txt) 聚合多个子模块。
- 当前有实际代码的模块主要是 [jungle-base](jungle-base/CMakeLists.txt) 和 [jungle-core](jungle-core/CMakeLists.txt)。
- 除非用户明确要求补齐脚手架，否则将 [jungle-api](jungle-api/CMakeLists.txt)、[jungle-client](jungle-client/CMakeLists.txt)、[jungle-server](jungle-server/CMakeLists.txt) 和 [jungle-ui](jungle-ui/CMakeLists.txt) 视为占位模块。

## 构建与测试

- 在仓库根目录配置：`cmake -S . -B build -G Ninja`
- 构建：`cmake --build build`
- 运行测试：`ctest --test-dir build --output-on-failure`
- 项目要求 C++23。非 Windows 主机上，顶层构建会优先选择 libc++ 与 lld 或 mold；不要随意移除这段逻辑。
- 已提交的 [build](build) 目录是生成产物。除非用户明确要求，否则不要编辑其中的文件。

## 模块边界

- [jungle-base](jungle-base/CMakeLists.txt) 负责基础类型、解析、哈希、panic 和 type-id 工具。
- [jungle-core](jungle-core/CMakeLists.txt) 当前负责 ECS 基元，并链接 `jungle::base`。
- 保持依赖单向流动：core 可以依赖 base，但 base 应保持不依赖 core。
- 遵循现有公共头文件前缀：
  - base 头文件：`jungle/...`
  - core 头文件：`jungle/core/...`

## 编辑约定

- 遵循 [.clang-format](.clang-format)。仓库使用 LLVM 风格格式化，4 空格缩进，列宽 110。
- 即使目标启用了预编译头，也要为当前文件直接使用的符号保留显式 include。PCH 配置见 [jungle-base/CMakeLists.txt](jungle-base/CMakeLists.txt) 和 [jungle-core/CMakeLists.txt](jungle-core/CMakeLists.txt)，但它不能替代 direct include。
- 保留现有的 `#pragma once`、命名空间布局和命名空间结束注释。
- 如果代码使用 `type_id`、`hash_val` 之类别名，先查看 [jungle-base/include/jungle/preusing.h](jungle-base/include/jungle/preusing.h)，不要重复引入同类别名。
- 链接时沿用当前库别名风格：`jungle::base`、`jungle::core`。

## 常见陷阱

- 不要根据空的模块 CMake 文件自行推断缺失架构；在发明 client/server/ui 结构前先确认需求。
- 根目录已经调用 `enable_testing()`，但当前没有真实测试目标。如果要补测试，请在 CMake 中显式接入，不要假定仓库已经选定某个测试框架。
- 文档发布工作流 [.github/workflows/mdbook.yaml](.github/workflows/mdbook.yaml) 会把两种语言的书都构建到 `docs/book`；修改文档结构时要保持与该布局兼容。
