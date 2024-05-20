use std::sync::Arc;
use std::{fs, io, str};

use nix::sys::signal::Signal::*;
use tokio::io::AsyncReadExt;
use tokio::net::{UnixListener, UnixStream};
use tokio::task;
use tokio::time::{sleep, Duration};

use resman_common::{QueueRequest, SOCKET_NAME};

mod shared_queue;
use shared_queue::JobQueue;

use nix::sys::signal::kill;
use nix::sys::wait::{waitpid, WaitStatus};
use nix::unistd::Pid;

async fn handle_client(mut stream: UnixStream, queue: Arc<JobQueue>) -> io::Result<()> {
    let mut buff = [0u8; 512];
    let mut full_str: String = "".to_string();

    loop {
        let n_bytes = stream.read(&mut buff).await?;
        if n_bytes == 0 {
            break;
        }

        let buff = match str::from_utf8(&buff[..n_bytes]) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("Invalid UTF-8 from client: {}", e);
                break;
            }
        };

        full_str.push_str(buff);
    }

    let request: QueueRequest = serde_json::from_str(&full_str)?;
    println!("Got data: {:?}", request);

    queue.enqueue(request).await;

    Ok(())
}

/* Ensure that a given Pid actually exists */
fn verify_process(pid: Pid) -> bool {
    match kill(pid, None) {
        Ok(_) => true,
        Err(e) => {
            eprintln!("Process with PID {} failed to verify: {}", pid, e);
            return false;
        }
    }
}

async fn dispatch_jobs(queue: Arc<JobQueue>) {
    loop {
        sleep(Duration::from_millis(1000)).await;

        let next_job = queue.dequeue().await;
        println!("[dispatcher] Got {:#?}", next_job);

        let pid = Pid::from_raw(next_job.pid);

        if verify_process(pid) {
            println!("[dispatcher] Running job: {}", next_job.pid);

            match kill(pid, SIGINT) {
                Ok(_) => {
                    println!("[dispatcher] Successfully sent SIGCONT.");
                }
                Err(e) => {
                    eprintln!("[dispatcher] Failed to SIGCONT: {}", e);
                    continue;
                }
            }

            // Loop until the given process has finished.
            // In some cases, waitpid returns when the process has merely been stopped, in which
            // case we need to call it again.
            loop {
                match waitpid(pid, None) {
                    Ok(stat) => {
                        println!("[dispatcher] Process {} has finished.", pid);
                        match stat {
                            WaitStatus::Exited(_, exit_code) => {
                                println!(
                                    "[dispatcher] Process {} exited with code {}.",
                                    pid, exit_code
                                );
                                break;
                            }
                            WaitStatus::Signaled(_, sig, _) => {
                                println!(
                                    "[dispatcher] Process {} exited due to signal {}.",
                                    pid, sig
                                );
                                break;
                            }
                            _ => {
                                println!(
                                    "[dispatcher] Process {} has not exited, but is stopped.",
                                    pid
                                );
                            }
                        }
                    }
                    Err(e) => {
                        eprintln!("[dispatcher] waitpid failed: {}", e);
                        continue;
                    }
                }
            }
        }
    }
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let job_queue = Arc::new(JobQueue::new());

    match fs::remove_file(SOCKET_NAME) {
        _ => {}
    }
    let listener = UnixListener::bind(SOCKET_NAME)?;

    println!("Server listening at {}", SOCKET_NAME);

    let queue_clone = job_queue.clone();
    task::spawn(async move {
        dispatch_jobs(queue_clone).await;
    });

    loop {
        match listener.accept().await {
            Ok((stream, _addr)) => {
                let queue_clone = job_queue.clone();
                task::spawn(async move {
                    if let Err(e) = handle_client(stream, queue_clone).await {
                        eprintln!("Error handling client: {}", e);
                    }
                });
            }
            Err(e) => {
                eprintln!("Error accepting connection: {}", e);
            }
        }
    }
}
