use std::sync::atomic::{self, AtomicBool};

pub struct StopToken {
    flag: AtomicBool,
}

impl StopToken {
    pub fn new() -> Self {
        StopToken {
            flag: AtomicBool::new(false),
        }
    }

    pub fn stop(&self) {
        self.flag.store(true, atomic::Ordering::Release);
    }

    pub fn stop_requested(&self) -> bool {
        self.flag.load(atomic::Ordering::Acquire)
    }
}
