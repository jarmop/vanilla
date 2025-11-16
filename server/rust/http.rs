use std::io::{Read, Write};
use std::net::{TcpListener};
use std::str;

fn main() -> std::io::Result<()> {
    let port = 3000;
    let address = format!("127.0.0.1:{}", port);
    let listener = TcpListener::bind(&address)?;
    println!("Listening on {}", address);

    // Accept one connection
    let (mut stream, addr) = listener.accept()?;
    println!("Client connected: {}", addr);

    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;

    if n > 0 {
        let received = str::from_utf8(&buf[..n]).unwrap_or("[invalid UTF-8]");
        println!("Received: {}", received);

        stream.write_all(b"Hello from server\n")?;
    }

    Ok(())
}