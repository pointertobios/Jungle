use std::{any::TypeId, collections::HashMap, sync::Arc};

use jungle_base::StringID;
use tokio::task::JoinSet;

use crate::{
    service::{self, Service},
    stop_token::StopToken,
};

pub struct Application {
    services: HashMap<TypeId, Box<dyn Service>>,

    stop_token: Arc<StopToken>,
}

impl Application {
    pub fn new(using_services: impl IntoIterator<Item = StringID>) -> Self {
        let mut services = HashMap::new();
        for name in using_services {
            let ctor = service::get_creator(name);
            let (k, v) = ctor();
            if let Some(v) = services.insert(k, v) {
                panic!("重复的服务 '{}'", v.name());
            }
        }
        Self {
            services,
            stop_token: Arc::new(StopToken::new()),
        }
    }

    pub fn get_service<S: 'static + Service>(&self) -> &dyn Service {
        &**self.services.get(&TypeId::of::<S>()).unwrap()
    }

    pub fn get_service_mut<S: 'static + Service>(&mut self) -> &mut dyn Service {
        &mut **self.services.get_mut(&TypeId::of::<S>()).unwrap()
    }

    pub async fn run(&mut self) {
        let mut set = JoinSet::new();
        let services = std::mem::take(&mut self.services);
        for (_, mut service) in services {
            let stop_token = Arc::clone(&self.stop_token);
            set.spawn(async move { service.run(&stop_token).await });
        }
        set.join_all().await;
    }
}
