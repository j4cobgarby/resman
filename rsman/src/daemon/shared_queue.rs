use resman_common::QueueRequest;
use std::collections::VecDeque;
use tokio::sync::{Mutex, Notify};
use rusqlite::{Connection, Result, OpenFlags};

const DB_FILE: &str = "./resman.db";

#[derive(Debug)]
pub struct Job {
    pub pid: i32,
    pub uid: u32,
    pub msg: String,
    pub cmd: String,
    pub time_submitted: u32,
    pub time_started: u32,
    pub time_finished: u32,
}

impl Job {
}

pub fn db_connect() -> Result<Connection> {
    let ret = match Connection::open_with_flags(DB_FILE, OpenFlags::SQLITE_OPEN_READ_WRITE | OpenFlags::SQLITE_OPEN_NO_MUTEX) {
        Ok(db) => db,
        Err(_) => {
            eprintln!("Database file {} could not be opened. Trying to create it...", DB_FILE);
            Connection::open_with_flags(DB_FILE,
                  OpenFlags::SQLITE_OPEN_READ_WRITE
                | OpenFlags::SQLITE_OPEN_CREATE
                | OpenFlags::SQLITE_OPEN_NO_MUTEX)?
        }
    };

    // Ensure that the jobs table exists
    ret.execute(
        "create table if not exists jobs (
            job_id integer primary key,
            uid integer not null,
            msg text,
            cmd text,
            time_submitted datetime,
            time_started datetime,
            time_finished datetime
        )", [])?;

    Ok(ret)
}

pub fn dequeue_job(conn: &mut Connection) -> Result<Option<Job>> {
    let conn = db_connect()?;
    let mut stmt = conn.prepare("select * from jobs")?;

    Ok(Some(Job {
        pid: 0,
        uid: 0,
        msg: String::from("Hello"),
        cmd: String::from("World"),
        time_submitted: 0,
        time_started: 0,
        time_finished: 0,
    }))
}

pub fn enqueue_job(conn: &mut Connection, job: Job) -> Result<()> {
    let conn = db_connect()?;

    Ok(())
}

pub fn find_job_by_id(conn: &mut Connection, job_id: i32) -> Result<Job> {
    todo!();
}

pub fn get_jobs(conn: &mut Connection, n: i32) -> Result<Vec<Job>> {
    todo!();
}
