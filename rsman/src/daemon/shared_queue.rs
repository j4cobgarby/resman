use resman_common::QueueRequest;

use std::collections::VecDeque;

use tokio::sync::{Mutex, Notify};

pub struct JobQueue {
    queue: Mutex<VecDeque<QueueRequest>>,
    notify: Notify,
}

impl JobQueue {
    pub fn new() -> Self {
        Self {
            queue: Mutex::new(VecDeque::new()),
            notify: Notify::new(),
        }
    }

    pub async fn enqueue(&self, job: QueueRequest) {
        let mut queue = self.queue.lock().await;
        queue.push_front(job);
        self.notify.notify_one();
    }

    pub async fn dequeue(&self) -> QueueRequest {
        loop {
            let mut queue = self.queue.lock().await;
            if let Some(job) = queue.pop_front() {
                return job;
            }
            drop(queue);
            self.notify.notified().await;
        }
    }

    pub async fn len(&self) -> usize {
        let queue = self.queue.lock().await;
        queue.len()
    }
}
