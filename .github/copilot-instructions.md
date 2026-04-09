# Project Guidelines

## Scope

- 只依据仓库中当前可验证的代码与配置做判断。
- 遇到依赖未来规划的事项时，先问用户确认，不要擅自判断。这包括引擎未来能力、.jaml schema、空壳 crate 的最终职责，以及任何 README 或代码里尚未定义的行为。

## Architecture

- 这是一个 Rust workspace。成员 crate 定义在 [../Cargo.toml](../Cargo.toml)。
- [../jungle-core/src/game.rs](../jungle-core/src/game.rs) 和 [../jungle-core/src/ecs/component.rs](../jungle-core/src/ecs/component.rs) 是 ECS 与运行时的主入口，修改引擎核心时优先参考这两处。
- [../jungle-macros/src/lib.rs](../jungle-macros/src/lib.rs) 提供 #[component(...)] 宏；组件相关代码通常同时涉及宏约束与 core 中的 trait 定义。
- [../jungle-example/src/main.rs](../jungle-example/src/main.rs) 是当前唯一能体现引擎接线方式的端到端示例。需要理解典型用法时，优先看 example，而不是猜测 cli、editor、client、server 这些 crate 的未来形态。
- 当前的 [../jungle-cli/src/main.rs](../jungle-cli/src/main.rs)、[../jungle-editor/src/main.rs](../jungle-editor/src/main.rs)、[../jungle-client/src/lib.rs](../jungle-client/src/lib.rs)、[../jungle-server/src/lib.rs](../jungle-server/src/lib.rs) 仍是占位实现。除非用户明确说明，否则不要把它们当成稳定边界或既定设计。

## Build And Test

- 常用工作区命令是 cargo check、cargo build、cargo test。
- 只有在任务明确要求验证，或改动确实需要编译反馈时，才主动运行这些命令；默认先做静态阅读和最小改动。
- 过程宏负向测试由 [../jungle-core/tests/component_macro_errors.rs](../jungle-core/tests/component_macro_errors.rs) 驱动，测试样例在 [../jungle-core/tests/ui/component_macro](../jungle-core/tests/ui/component_macro)。如果改了 #[component(...)] 宏或其约束，记得同步考虑这些用例。

## Conventions

- 所有 crate 当前都声明 edition = "2024"。这是仓库现状，不要自行推断它对应的工具链约束；如果任务涉及 edition、MSRV 或工具链兼容性，先问用户确认。
- workspace 根配置使用 resolver = "3"，并在 dev.build-override 中设置 opt-level = 3。调整构建配置时先保留这一现状，除非用户明确要求修改。
- 添加依赖时不要直接编辑 Cargo.toml；应使用 cargo add 命令更新依赖声明，除非用户明确要求手动修改清单文件。
- ComponentManager 的向下转型必须走 trait 对象上的 as_any 或 as_any_mut，参考 [../jungle-core/src/game.rs](../jungle-core/src/game.rs) 中 get_manager 与 get_manager_mut 的写法；不要对 Box<dyn ComponentManager> 本身做错误的 Any 转型。
- 新组件应遵循 [../jungle-core/src/ecs/components/node.rs](../jungle-core/src/ecs/components/node.rs) 展示的模式：命名 struct、包含 entity: Entity 字段，并使用 #[component(storage = ..., exclusive = ...)] 标注。
- 资源与场景相关格式目前以 [../jungle-example/jungle.proj.toml](../jungle-example/jungle.proj.toml)、[../jungle-example/ast/assets.toml](../jungle-example/ast/assets.toml)、[../jungle-example/ast/prefabs/example.prefab.jaml](../jungle-example/ast/prefabs/example.prefab.jaml)、[../jungle-example/ast/scenes/initial.scene.jaml](../jungle-example/ast/scenes/initial.scene.jaml) 为例。仓库里没有完整 schema 文档，因此修改解析、生成或字段语义前，先问用户确认预期。
- README 目前只有一句项目描述，缺少可依赖的详细文档。遇到约定不明确时，以代码为准，并把需要确认的问题直接提给用户。
