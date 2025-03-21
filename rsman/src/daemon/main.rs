use std::sync::Arc;
use std::{fs, io, str};

use nix::sys::signal::Signal::*;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{UnixListener, UnixStream};
use tokio::task;
use tokio::time::{sleep, Duration};

use resman_common::{IPCMessage, QueueRequest, QueueResponse, SkipResponse, SOCKET_NAME};

mod shared_queue;
use shared_queue::*;

use nix::sys::signal::kill;
use nix::sys::wait::{waitpid, WaitStatus};
use nix::unistd::Pid;

async fn handle_client(mut stream: UnixStream) -> io::Result<()> {
    println!("Handling client.");
    let mut buff: String = String::new();

    stream.read_to_string(&mut buff).await?;

    let request: IPCMessage = serde_json::from_str(&buff)?;
    println!("Got data: {:?}", request);

    match request {
        IPCMessage::QueueReq(req) => {
            println!("Got request: {:?}", req);
            let pid = Pid::from_raw(req.pid);
            kill(pid, SIGUSR1)?;
            println!("Sent signal.");
        }

        IPCMessage::SkipReq(req) => {
            let resp = SkipResponse { success: true };
        }

        IPCMessage::StatReq(req) => {}
    }

    // queue.enqueue(request).await;

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

async fn schedule() -> Option<Job> {
    let next_job: Job = match dequeue_job() {
        Ok(Some(job)) => job,
        _ => return None,
    };

    println!("[scheduler] Got {:#?}", next_job);

    let pid = Pid::from_raw(next_job.pid);
    if verify_process(pid) {
        println!("[scheduler] Starting job with PID {}", next_job.pid);
        match kill(pid, SIGUSR1) {
            Ok(_) => println!("[scheduler] Signal sent properly"),
            Err(e) => {
                eprintln!("[scheduler] Failed to send SIGUSR1: {}", e);
                return None;
            }
        }
    }

    Some(next_job)
}

async fn dispatch_jobs() {
    loop {
        sleep(Duration::from_millis(1000)).await;

        let next_job: Job = match dequeue_job() {
            Ok(Some(job)) => job,
            Ok(None) => {
                continue;
            }
            Err(e) => {
                eprintln!("Failed to get job: {}", e.to_string());
                continue;
            }
        };

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
    match fs::remove_file(SOCKET_NAME) {
        _ => {}
    }
    let listener = UnixListener::bind(SOCKET_NAME)?;

    println!("Server listening at {}", SOCKET_NAME);

    loop {
        match listener.accept().await {
            Ok((stream, _addr)) => {
                task::spawn(async move {
                    if let Err(e) = handle_client(stream).await {
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
