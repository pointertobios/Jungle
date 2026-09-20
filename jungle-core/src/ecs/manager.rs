use std::{any::TypeId, collections::HashMap, sync::LazyLock};

use jungle_base::StringID;

use crate::ecs::component::Component;

pub type ManagerCreator = fn() -> (TypeId, Box<dyn Manager>);

pub struct ManagerCreatorRegistration {
    pub name: StringID,
    pub creator: ManagerCreator,
}

inventory::collect!(ManagerCreatorRegistration);

static MANAGERS_BY_NAME: LazyLock<HashMap<StringID, ManagerCreator>> = LazyLock::new(|| {
    let mut map = HashMap::new();
    for reg in inventory::iter::<ManagerCreatorRegistration> {
        if map.insert(reg.name, reg.creator).is_some() {
            panic!("组件名称重复注册");
        }
    }
    map
});

pub fn get_creator(name: StringID) -> &'static ManagerCreator {
    MANAGERS_BY_NAME.get(&name).unwrap()
}

pub fn manager_creator_f<M: 'static + Manager + Default, C: 'static + Component>()
-> (TypeId, Box<dyn Manager>) {
    (TypeId::of::<C>(), Box::new(M::default()))
}

#[macro_export]
macro_rules! register_component {
    ($ty: ty) => {
        inventory::submit! {
            $crate::ecs::manager::ManagerCreatorRegistration {
                name: jungle_base::StringID::of(std::stringify!($ty)),
                creator: $crate::ecs::manager::manager_creator_f::<$crate::ecs::manager::ManagerImpl<$ty>, $ty>,
            }
        }
    };
}

pub trait Manager {}

pub struct ManagerImpl<T: Component> {
    storage: T::Storage,
}

impl<T: Component> Manager for ManagerImpl<T> {}
