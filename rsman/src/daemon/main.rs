use std::io;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{UnixListener, UnixStream};
use tokio::task;

async fn handle_client(mut stream: UnixStream) -> io::Result<()> {
    let mut buffer = [0u8; 1024];

    loop {
        let bytes_read = stream.read(&mut buffer).await?;

        if bytes_read == 0 {
            break;
        }

        println!("Received: {:?}", &buffer[..bytes_read]);

        // Echo the data back to the client
        stream.write_all(&buffer[..bytes_read]).await?;
    }

    Ok(())
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let listener = UnixListener::bind(resman_common::SOCKET_NAME)?;

    println!("Server listening at {}", resman_common::SOCKET_NAME);

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
