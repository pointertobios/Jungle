# Jungle

现代化、工业化的游戏引擎。

## Roadmap

### Phase 1: 稳定核心运行时

目标：把 jungle-core 从“能跑起来的雏形”推进到“可持续扩展的最小引擎内核”。

- 补齐 GameCore 的主循环职责，明确 pre_tick、tick、post_tick 的调度入口与执行顺序。
- 为组件增删、pending 队列处理、生命周期钩子建立更明确的运行时约束与测试覆盖。
- 为 Entity、ComponentManager、Storage 抽象补充更完整的增删查改能力与行为测试。

### Phase 2: 完成资产与场景闭环

目标：让示例中的 project/assets/prefab/scene 不只是静态文件，而是能被正式加载与实例化。

- 实现 scene.rs 与 prefab.rs，建立场景、预制体、实体树之间的正式加载接口。
- 打通 jungle.proj.toml、assets.toml、\*.scene.jaml、\*.prefab.jaml 的读取流程。
- 定义从 EntityTree 到运行时实体/组件实例的构建过程。
- 明确 prefab-ref 与 prefab 的运行时语义，并补充对应测试与示例。

### Phase 3: 丰富基础组件与系统模型

目标：在已有 Node、Transform、PrefabRef 之上，形成可继续堆叠玩法逻辑的基础层。

- 继续补充通用组件，例如可见性、层级关系、激活状态、标签或名称访问接口。
- 引入或完善 system 抽象，明确系统如何读取组件、如何参与 tick。
- 梳理 Node 与 Transform 的关系，明确父子节点、局部变换、世界变换等后续扩展方向。
- 为组件序列化与 JAML 映射建立更稳定的约束，减少未来格式变更成本。

### Phase 4: 建立开发工具链

目标：先做出可用的工程工具，再考虑复杂编辑器能力。

- 先让 jungle-cli 承担实际职责，例如项目检查、资源验证、场景导入导出、测试辅助命令。
- 为 README 与示例补充最小开发流程，降低新功能验证成本。
- 在 editor 仍未成型前，优先保证 CLI 与资源格式足够稳定，避免过早投入 GUI 外壳。

### Phase 5: 明确客户端 / 服务端边界

目标：等核心与资产层稳定后，再展开多端运行时设计，避免占位 crate 提前固化错误边界。

- 先定义 client/server 共享的数据结构、组件同步边界与资源职责。
- 再决定 jungle-client 与 jungle-server 是薄封装、独立运行时，还是仅作为集成层。
- 结合 AssetEnv 中的 client_only / server_only 语义，补足跨端场景装载策略。

### Phase 6: 文档与示例工程化

目标：把现有代码中的隐含约定变成可维护的公开约定。

- 补充组件宏、组件存储、JAML 资源格式、示例项目结构的文档。
- 为 jungle-example 增加更接近真实使用方式的样例，而不只是启动 GameCore。
- 为关键模块补充“什么已经稳定、什么仍在演进中”的说明，减少后续误用。

## 建议的近期优先级

如果按当前代码基础推进，建议优先级如下：

1. 修复测试基线，确保核心回归检查可信。
2. 实现 scene/prefab 正式 API，打通资源到运行时实体的闭环。
3. 完善 GameCore 调度与 system/component 生命周期。
4. 给 CLI 增加最小可用工具能力。
5. 在核心边界稳定后，再推进 editor/client/server。
