use clap::{Parser, Subcommand};
use nix::unistd::{ForkResult, Pid, Uid};
use resman_common::*;
use std::ffi::{CString, NulError};
use std::io;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::UnixStream;
use tokio::signal::unix::{signal, SignalKind};

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    subcommand: Subcommands,
}

#[derive(Subcommand, Debug)]
enum Subcommands {
    #[command(aliases = &["c", "ch", "chk"], about = "Get reservation status.")]
    Check {
        #[arg(short, long, help = "Confirm that user has read notice.")]
        interactive: bool,

        #[arg(short, long, help = "Silence output; only set exit status.")]
        silent: bool,
    },

    #[command(aliases = &["x", "e", "ex", "exe"], about = "Queue a command.")]
    Exec {
        #[arg(short, long, help = "Short message to show to others.")]
        msg: Option<String>,

        #[arg(trailing_var_arg = true, help = "The command to queue.")]
        args: Vec<String>,
    },

    #[command(aliases = &["s", "sk"], about = "Skip a job.")]
    Skip {
        #[arg(short, long, help = "Job ID to skip. Omit to skip current job.")]
        job_id: Option<i32>,

        #[arg(short, long, help = "Skip even other users' jobs.")]
        force: bool,
    },
}

fn to_sh_args(args: &Vec<String>) -> Vec<CString> {
    vec![
        CString::new("/usr/bin/sh").expect("Failure"),
        CString::new("-c").expect("Failure"),
        CString::new(args.join(" ")).expect("Failure"),
    ]
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let args = Args::parse();

    // let stream = UnixStream::connect(SOCKET_NAME);

    match &args.subcommand {
        Subcommands::Exec { msg, args } => {
            let mut stream = match UnixStream::connect(SOCKET_NAME).await {
                Ok(s) => s,
                Err(e) => {
                    panic!("Failed to connect to daemon: {}", e.to_string());
                }
            };

            let req = QueueRequest {
                pid: Pid::this().as_raw(),
                uid: Uid::current().as_raw(),
                msg: match msg {
                    Some(msg) => msg.to_owned(),
                    None => String::from(""),
                },
                cmd: args.join(" "),
            };

            let req = IPCMessage::QueueReq(req);
            let req = serde_json::to_string(&req).unwrap();

            stream.write_all(req.as_bytes()).await?;
            println!("Written data to daemon");
            stream.shutdown().await?;

            let mut sig = signal(SignalKind::user_defined1())?;
            sig.recv().await;
            println!("Got signal!!");

            match unsafe { nix::unistd::fork() } {
                Ok(ForkResult::Parent { child }) => {
                    println!("[resman] Making your job! PID = {}", child);
                    loop {
                        let stat = nix::sys::wait::waitpid(child, None).unwrap();
                    }
                    println!("[resman] Child process ended!");
                }
                Ok(ForkResult::Child) => {
                    nix::unistd::execvp(
                        &CString::new("sh").expect("Failed to make to c str"),
                        &to_sh_args(args)[..],
                    )
                    .expect("Failed to exec");
                }
                Err(_) => println!("Failed to fork."),
            }
        }

        Subcommands::Check {
            interactive,
            silent,
        } => {
            let req = IPCMessage::StatReq(StatusRequest {});
            let req = serde_json::to_string(&req).unwrap();

            println!("Serialised = {}", req);

            let stream = UnixStream::connect(SOCKET_NAME);
        }

        Subcommands::Skip { job_id, force } => {
            let req = IPCMessage::SkipReq(SkipRequest {
                job_id: *job_id,
                uid: Uid::current().as_raw(),
                force: *force,
            });

            let req = serde_json::to_string(&req).unwrap();

            println!("Serialised = {}", req);

            let stream = UnixStream::connect(SOCKET_NAME);
        }
    }

    Ok(())
}
