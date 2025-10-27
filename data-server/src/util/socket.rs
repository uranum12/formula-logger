use std::{thread, time::Duration};

use anyhow::{Context, Result};
use nng::options::Options;
use nng::options::protocol::pubsub::Subscribe;
use nng::{Protocol, Socket};

pub fn init_pub(socket_addr: &str) -> Result<Socket> {
    let socket = Socket::new(Protocol::Pub0).context("Failed to create pub socket")?;
    socket
        .listen(socket_addr)
        .with_context(|| format!("Failed to listen socket on {socket_addr}"))?;

    Ok(socket)
}

pub fn init_sub(socket_addrs: &[&str]) -> Result<Socket> {
    let socket = Socket::new(Protocol::Sub0).context("Failed to create sub socket")?;
    socket
        .set_opt::<Subscribe>(b"".to_vec())
        .context("Failed to subscribe")?;

    for addr in socket_addrs {
        loop {
            match socket.dial(&addr) {
                Ok(_) => {
                    println!("Success to dial to {addr}");
                    break;
                }
                Err(e) => {
                    eprintln!("Failed to dial to {addr}: {e}");
                    thread::sleep(Duration::from_secs(1));
                }
            }
        }
    }

    Ok(socket)
}
