use serde::{Deserialize, Serialize};

pub const SOCKET_NAME: &str = "/tmp/resman_socket";

#[derive(Debug, Serialize, Deserialize)]
pub enum IPCMessage {
    QueueReq(QueueRequest),
    SkipReq(SkipRequest),
    StatReq(StatusRequest),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct QueueRequest {
    pub pid: i32,
    pub uid: u32,
    pub msg: String,
    pub cmd: String, // it's not run by the server, but sent out of interest
}

#[derive(Debug, Serialize, Deserialize)]
pub struct QueueResponse {
    pub success: bool,
    pub place_in_queue: i32,
    pub job_id: i32,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct SkipRequest {
    pub job_id: Option<i32>,
    pub uid: u32,
    pub force: bool,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct SkipResponse {
    pub success: bool,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct StatusRequest {}

#[derive(Debug, Serialize, Deserialize)]
pub struct StatusResponse {
    pub jobs: Vec<JobStatus>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct JobStatus {
    pub job_id: i32,
    pub uid: u32,
    pub msg: String,
    pub elapsed_seconds: i32,
}
