use std::{collections::HashMap, time::Duration};

use tokio::{
    runtime::{self, Runtime},
    time::interval,
};

use crate::ecs::{
    component::{Component, ComponentId, ComponentManager, ComponentManagerImpl},
    components::{node::Node, prefab_ref::PrefabRef},
};

pub trait Game {
    fn run(self) -> anyhow::Result<()>;

    fn get_managers(&self) -> &HashMap<ComponentId, Box<dyn ComponentManager>>;
    fn get_managers_mut(&mut self) -> &mut HashMap<ComponentId, Box<dyn ComponentManager>>;
    fn get_name_to_type(&self) -> &HashMap<String, ComponentId>;
    fn get_name_to_type_mut(&mut self) -> &mut HashMap<String, ComponentId>;

    fn register_manager<C: Component>(&mut self) {
        self.get_name_to_type_mut()
            .insert(C::type_name().to_string(), C::id());
        self.get_managers_mut()
            .insert(C::id(), Box::new(ComponentManagerImpl::<C>::new()));
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

pub struct GameCore {
    managers: HashMap<ComponentId, Box<dyn ComponentManager>>,
    name_to_type: HashMap<String, ComponentId>,

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
}

impl Game for GameCore {
    fn run(self) -> anyhow::Result<()> {
        self.tokio_runtime.block_on(async {
            let mut itv = interval(self.game_tick);
            loop {
                itv.tick().await;
            }
        })
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
