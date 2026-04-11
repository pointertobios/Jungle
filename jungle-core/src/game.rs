use std::{collections::HashMap, marker::PhantomData, path::PathBuf, sync::Arc, time::Duration};

use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use tokio::{
    runtime::{self, Runtime},
    time::interval,
};

use crate::{
    asset::{AssetManager, AssetPackage},
    ecs::{
        component::{
            Component, ComponentId, ComponentManager, ComponentManagerImpl, ComponentSerdeHelper,
            ComponentSerdeHelperImpl,
        },
        components::{node::Node, prefab_ref::PrefabRef, transform::Transform},
    },
    project::Project,
};

pub trait Game {
    fn run(self) -> anyhow::Result<()>;

    fn get_serdehelpers(&self) -> &HashMap<ComponentId, Box<dyn ComponentSerdeHelper>>;
    fn get_serdehelpers_mut(&mut self) -> &mut HashMap<ComponentId, Box<dyn ComponentSerdeHelper>>;
    fn get_managers(&self) -> &HashMap<ComponentId, Box<dyn ComponentManager>>;
    fn get_managers_mut(&mut self) -> &mut HashMap<ComponentId, Box<dyn ComponentManager>>;
    fn get_name_to_type(&self) -> &HashMap<String, ComponentId>;
    fn get_name_to_type_mut(&mut self) -> &mut HashMap<String, ComponentId>;

    fn register_manager_noserde<C: Component>(&mut self) {
        self.get_name_to_type_mut()
            .insert(C::type_name().to_string(), C::id());
        self.get_managers_mut()
            .insert(C::id(), Box::new(ComponentManagerImpl::<C>::new()));
    }

    fn register_manager<C: Component + Serialize + for<'de> Deserialize<'de>>(&mut self) {
        self.register_manager_noserde::<C>();
        self.get_serdehelpers_mut().insert(
            C::id(),
            Box::new(ComponentSerdeHelperImpl::<C>(PhantomData)) as Box<dyn ComponentSerdeHelper>,
        );
    }

    fn get_manager<C: Component>(&self) -> Option<&ComponentManagerImpl<C>> {
        self.get_managers().get(&C::id()).map(|manager| {
            manager
                .as_any()
                .downcast_ref::<ComponentManagerImpl<C>>()
                .unwrap()
        })
    }

    fn get_manager_mut<C: Component>(&mut self) -> Option<&mut ComponentManagerImpl<C>> {
        self.get_managers_mut().get_mut(&C::id()).map(|manager| {
            manager
                .as_any_mut()
                .downcast_mut::<ComponentManagerImpl<C>>()
                .unwrap()
        })
    }

    fn get_manager_by_name(&self, name: &str) -> Option<&dyn ComponentManager> {
        self.get_name_to_type()
            .get(name)
            .cloned()
            .and_then(|id| self.get_managers().get(&id).map(|boxv| boxv.as_ref()))
    }

    fn get_manager_by_name_mut(
        &mut self,
        name: &str,
    ) -> Option<&mut (dyn ComponentManager + 'static)> {
        self.get_name_to_type_mut()
            .get(name)
            .cloned()
            .and_then(move |id| {
                self.get_managers_mut()
                    .get_mut(&id)
                    .map(|boxv| boxv.as_mut())
            })
    }
}

pub struct CoreConfig {
    pub project_dir: PathBuf,
    pub game_tick: Duration,
}

impl Default for CoreConfig {
    fn default() -> Self {
        Self {
            project_dir: PathBuf::from("."),
            game_tick: Duration::from_millis(10),
        }
    }
}

pub struct GameCore {
    serdehelpers: HashMap<ComponentId, Box<dyn ComponentSerdeHelper>>,
    managers: HashMap<ComponentId, Box<dyn ComponentManager>>,
    name_to_type: HashMap<String, ComponentId>,

    project_dir: PathBuf,

    game_tick: Duration,

    tokio_runtime: Runtime,
}

impl GameCore {
    pub fn new(config: CoreConfig) -> Self {
        let mut res = Self {
            serdehelpers: HashMap::new(),
            managers: HashMap::new(),
            name_to_type: HashMap::new(),
            project_dir: config.project_dir,
            game_tick: config.game_tick,
            tokio_runtime: runtime::Builder::new_multi_thread()
                .enable_all()
                .build()
                .unwrap(),
        };
        res.register_manager_noserde::<Node>();
        res.register_manager_noserde::<PrefabRef>();
        res.register_manager::<Transform>();
        res
    }
}

impl Game for GameCore {
    fn run(self) -> Result<()> {
        let game_tick = self.game_tick;
        let (project, asset_manager) = self.tokio_runtime.block_on(async {
            let project = Project::from(&self.project_dir).await.with_context(|| {
                format!(
                    "failed to load project from {}",
                    self.project_dir.clone().display()
                )
            })?;
            let asset_manager = Arc::new(
                AssetManager::new(AssetPackage::FileSystem(
                    self.project_dir.join(project.assets.clone()),
                ))
                .await
                .with_context(|| {
                    format!(
                        "failed to initialize asset manager with asset root {}",
                        project.assets.display()
                    )
                })?,
            );
            Result::<(Project, Arc<AssetManager>)>::Ok((project, asset_manager))
        })?;

        println!("{:#?}", *asset_manager);

        let hdl = self.tokio_runtime.spawn(async move {
            let mut itv = interval(game_tick);
            loop {
                itv.tick().await;
            }
        });
        self.tokio_runtime.block_on(async { hdl.await? })
    }

    fn get_serdehelpers(&self) -> &HashMap<ComponentId, Box<dyn ComponentSerdeHelper>> {
        &self.serdehelpers
    }

    fn get_serdehelpers_mut(&mut self) -> &mut HashMap<ComponentId, Box<dyn ComponentSerdeHelper>> {
        &mut self.serdehelpers
    }

    fn get_managers(&self) -> &HashMap<ComponentId, Box<dyn ComponentManager>> {
        &self.managers
    }

    fn get_managers_mut(&mut self) -> &mut HashMap<ComponentId, Box<dyn ComponentManager>> {
        &mut self.managers
    }

    fn get_name_to_type(&self) -> &HashMap<String, ComponentId> {
        &self.name_to_type
    }

    fn get_name_to_type_mut(&mut self) -> &mut HashMap<String, ComponentId> {
        &mut self.name_to_type
    }
}
