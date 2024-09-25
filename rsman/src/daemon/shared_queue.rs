use rusqlite::{Connection, OpenFlags, Result};
use std::time::{SystemTime, UNIX_EPOCH};

const DB_FILE: &str = "./resman.db";

#[derive(Debug)]
pub struct Job {
    pub job_id: i32,
    pub pid: i32,
    pub uid: u32,
    pub msg: String,
    pub cmd: String,
    pub time_submitted: u64,
    pub time_started: u64,
    pub time_finished: u64,
}

impl Job {}

pub fn db_connect() -> Result<Connection> {
    let ret = match Connection::open_with_flags(
        DB_FILE,
        OpenFlags::SQLITE_OPEN_READ_WRITE | OpenFlags::SQLITE_OPEN_NO_MUTEX,
    ) {
        Ok(db) => db,
        Err(_) => {
            eprintln!(
                "Database file {} could not be opened. Trying to create it...",
                DB_FILE
            );
            Connection::open_with_flags(
                DB_FILE,
                OpenFlags::SQLITE_OPEN_READ_WRITE
                    | OpenFlags::SQLITE_OPEN_CREATE
                    | OpenFlags::SQLITE_OPEN_NO_MUTEX,
            )?
        }
    };

    // Ensure that the jobs table exists
    ret.execute(
        "create table if not exists jobs (
            job_id integer primary key,
            pid integer not null,
            uid integer not null,
            msg text,
            cmd text,
            time_submitted integer,
            time_started integer default 0,
            time_finished integer default 0
        )",
        [],
    )?;

    Ok(ret)
}

fn current_timestamp() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("Time went backwards?")
        .as_secs()
}

pub fn dequeue_job() -> Result<Option<Job>> {
    let conn = db_connect()?;
    let mut stmt = conn.prepare(
        "select * from jobs
         where time_started = 0
         order by time_submitted
         limit 1",
    )?;

    let mut rows = stmt.query([])?;
    match rows.next() {
        Ok(Some(row)) => {
            let row_id: i32 = row.get(0)?;

            conn.execute(
                "update jobs set time_started = ?1 where job_id = ?2",
                (current_timestamp(), row_id),
            )?;

            Ok(Some(Job {
                job_id: row_id,
                pid: row.get(0)?,
                uid: row.get(2)?,
                msg: row.get(3)?,
                cmd: row.get(4)?,
                time_submitted: row.get(5)?,
                time_started: row.get(6)?,
                time_finished: row.get(7)?,
            }))
        }

        Ok(None) => Ok(None),
        Err(e) => Err(e),
    }
}

pub fn enqueue_job(job: Job) -> Result<()> {
    let conn = db_connect()?;
    conn.execute(
        "insert into jobs (pid, uid, msg, cmd, time_submitted) values (?1, ?2, ?3, ?4, ?5)",
        (job.pid, job.uid, job.msg, job.cmd, current_timestamp()),
    )?;

    Ok(())
}

pub fn find_job_by_id(job_id: i32) -> Result<Job> {
    todo!();
}

pub fn get_jobs(n: i32) -> Result<Vec<Job>> {
    todo!();
}
