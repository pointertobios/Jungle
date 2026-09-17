use std::{any::TypeId, collections::HashMap, sync::LazyLock};

use async_trait::async_trait;
use jungle_base::StringID;

use crate::stop_token::StopToken;

pub type ServiceCreator = fn() -> (TypeId, Box<dyn Service>);

pub struct ServiceRegistration {
    pub name: StringID,
    pub creator: ServiceCreator,
}

inventory::collect!(ServiceRegistration);

static SERVICES_BY_NAME: LazyLock<HashMap<StringID, ServiceCreator>> = LazyLock::new(|| {
    let mut map = HashMap::new();
    for reg in inventory::iter::<ServiceRegistration> {
        map.insert(reg.name, reg.creator);
    }
    map
});

pub fn get_creator(name: StringID) -> &'static ServiceCreator {
    SERVICES_BY_NAME.get(&name).unwrap()
}

pub fn service_creator_f<S: 'static + Service + Default>() -> (TypeId, Box<dyn Service>) {
    (TypeId::of::<S>(), Box::new(S::default()))
}

#[macro_export]
macro_rules! register_service {
    ($ty: ty) => {
        inventory::submit! {
            $crate::service::ServiceRegistration {
                name: jungle_base::StringID::of(std::stringify!($ty)),
                creator: $crate::service::service_creator_f::<$ty>,
            }
        }
    };
}

#[async_trait]
pub trait Service: Send + Sync {
    fn name(&self) -> &'static str;

    async fn run(&mut self, st: &StopToken);
}
