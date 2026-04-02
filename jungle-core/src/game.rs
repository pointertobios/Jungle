use std::{any::TypeId, collections::HashMap, time::Duration};

use tokio::{
    runtime::{self, Runtime},
    time::interval,
};

use crate::ecs::{
    component::{Component, ComponentManager, ComponentManagerImpl},
    components::{node::Node, prefab_ref::PrefabRef},
};

pub struct GameCore {
    managers: HashMap<TypeId, Box<dyn ComponentManager>>,
    name_to_type: HashMap<String, TypeId>,

    game_tick: Duration,

    tokio_runtime: Runtime,
}

impl GameCore {
    pub fn new(game_tick: Duration) -> Self {
        let mut res = Self {
            managers: HashMap::new(),
            name_to_type: HashMap::new(),
            game_tick,
            tokio_runtime: runtime::Builder::new_multi_thread()
                .enable_all()
                .build()
                .unwrap(),
        };
        res.register_manager::<Node>();
        res.register_manager::<PrefabRef>();
        res
    }

    pub fn run(self) -> anyhow::Result<()> {
        self.tokio_runtime.block_on(async {
            let mut itv = interval(self.game_tick);
            loop {
                itv.tick().await;
            }
        })
    }

    pub fn register_manager<C: Component>(&mut self) {
        self.name_to_type
            .insert(C::default().type_name().to_string(), TypeId::of::<C>());
        self.managers.insert(
            TypeId::of::<C>(),
            Box::new(ComponentManagerImpl::<C>::new()),
        );
    }

    pub fn get_manager<C: Component>(&self) -> Option<&ComponentManagerImpl<C>> {
        let type_id = TypeId::of::<C>();
        if let Some(manager) = self.managers.get(&type_id) {
            manager.as_any().downcast_ref::<ComponentManagerImpl<C>>()
        } else {
            None
        }
    }

    pub fn get_manager_mut<C: Component>(&mut self) -> Option<&mut ComponentManagerImpl<C>> {
        let type_id = TypeId::of::<C>();
        if let Some(manager) = self.managers.get_mut(&type_id) {
            manager
                .as_any_mut()
                .downcast_mut::<ComponentManagerImpl<C>>()
        } else {
            None
        }
    }
}
