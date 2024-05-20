use clap::{Parser, Subcommand};
use resman_common::*;
use std::io;
use tokio::net::UnixStream;

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

    #[command(aliases = &["s", "sk"], about = "Skip the current job.")]
    Skip {
        #[arg(short, long, help = "Skip even other users' jobs.")]
        force: bool,
    },
}

pub fn main() -> io::Result<()> {
    let args = Args::parse();

    // let stream = UnixStream::connect(SOCKET_NAME);

    match &args.subcommand {
        Subcommands::Exec { msg, args } => {
            println!("Executing command: {:?}", args);

            let req: QueueRequest = QueueRequest {
                pid: 1337,
                uid: 2,
                msg: match msg {
                    Some(msg) => msg.to_owned(),
                    None => String::from(""),
                },
                cmd: args.join(" "),
            };

            let req = IPCMessage::QueueReq(req);
            let req = serde_json::to_string(&req).unwrap();

            println!("Serialised = {}", req);

            let stream = UnixStream::connect(SOCKET_NAME);
        }

        Subcommands::Check {
            interactive,
            silent,
        } => {
            println!("Server is reserved?: ");

            let req: StatusRequest = StatusRequest {};
        }

        Subcommands::Skip { force } => {
            println!("Skipping job...");

            let req: SkipRequest = SkipRequest { job_id: 10 };
        }
    }

    Ok(())
}
