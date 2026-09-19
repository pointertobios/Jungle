use std::sync::atomic::{self, AtomicBool};

use tokio::sync::Notify;

pub struct ServiceController {
    stop_flag: AtomicBool,
    notify: Notify,
}

impl ServiceController {
    pub fn new() -> Self {
        ServiceController {
            stop_flag: AtomicBool::new(false),
            notify: Notify::new(),
        }
    }

    pub fn stop(&self) {
        self.stop_flag.store(true, atomic::Ordering::Release);
        self.notify.notify_waiters();
    }

    pub fn stop_requested(&self) -> bool {
        self.stop_flag.load(atomic::Ordering::Acquire)
    }

    pub async fn wait_for_stop(&self) {
        while !self.stop_flag.load(atomic::Ordering::Acquire) {
            self.notify.notified().await;
        }
    }

    pub async fn wait_for_awake(&self) {
        self.notify.notified().await;
    }
}
