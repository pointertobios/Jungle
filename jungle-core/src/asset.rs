use std::num::NonZeroU128;

pub mod ecs;
pub mod env;
pub mod prefab;
pub mod scene;

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Hash)]
pub struct AssetId(Option<NonZeroU128>);
