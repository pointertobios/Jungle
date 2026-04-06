use jungle_macros::component;
use nalgebra::Vector3;

use crate::ecs::{
    component::{ComponentExt, ContinuousComponentStorage},
    entity::Entity,
};

#[component(storage = ContinuousComponentStorage, exclusive = true)]
#[derive(Debug, Default, PartialEq)]
pub struct Transform {
    entity: Entity,

    #[serde((f32, f32, f32), |v| (v.x, v.y, v.z), |&(x, y, z)| Vector3::new(x, y, z))]
    position: Vector3<f32>,
    #[serde((f32, f32, f32), |v| (v.x, v.y, v.z), |&(x, y, z)| Vector3::new(x, y, z))]
    rotation: Vector3<f32>,
    #[serde((f32, f32, f32), |v| (v.x, v.y, v.z), |&(x, y, z)| Vector3::new(x, y, z))]
    scale: Vector3<f32>,
}

impl ComponentExt for Transform {}
