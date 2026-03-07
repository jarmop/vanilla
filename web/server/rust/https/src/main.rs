use std::fs::File;
use std::io::{Read, Write};
use std::net::TcpListener;
use std::sync::Arc;

use rustls::{Certificate, PrivateKey, ServerConfig};
use rustls::server::ServerConnection;
use rustls_pemfile::{certs, pkcs8_private_keys, rsa_private_keys};
use rustls::StreamOwned;

fn load_certs(path: &str) -> Vec<Certificate> {
    let mut f = File::open(path).expect("cannot open cert file");
    let mut data = Vec::new();
    f.read_to_end(&mut data).expect("read cert");
    let mut reader = &data[..];
    certs(&mut reader)
        .expect("parse certs")
        .into_iter()
        .map(Certificate)
        .collect()
}

fn load_private_key(path: &str) -> PrivateKey {
    let mut f = File::open(path).expect("cannot open key file");
    let mut data = Vec::new();
    f.read_to_end(&mut data).expect("read key");
    // try PKCS8 first (modern)
    let mut reader = &data[..];
    if let Ok(mut keys) = pkcs8_private_keys(&mut reader) {
        if !keys.is_empty() {
            return PrivateKey(keys.remove(0));
        }
    }
    // fall back to RSA keys
    let mut reader2 = &data[..];
    let mut keys = rsa_private_keys(&mut reader2).expect("parse rsa keys");
    if !keys.is_empty() {
        return PrivateKey(keys.remove(0));
    }
    panic!("no private keys found in {}", path);
}

fn main() {
    // FILES: cert.pem and key.pem in the current dir
    let certs = load_certs("../../cert.pem");
    let key = load_private_key("../../key.pem");

    let config = ServerConfig::builder()
        .with_safe_defaults()
        .with_no_client_auth()
        .with_single_cert(certs, key)
        .expect("bad cert/key");
    let config = Arc::new(config);

    let listener = TcpListener::bind("127.0.0.1:8443").expect("bind");
    println!("Listening on https://localhost:8443");

    for stream in listener.incoming() {
        match stream {
            Ok(tcp_stream) => {
                // Each connection gets its own rustls ServerConnection
                let server_conn = ServerConnection::new(config.clone()).expect("server conn");
                let mut tls_stream = StreamOwned::new(server_conn, tcp_stream);

                // Read up to 8KiB of request (very minimal, not robust)
                let mut buf = [0u8; 8192];
                match tls_stream.read(&mut buf) {
                    Ok(n) if n > 0 => {
                        println!("Request:\n{}", String::from_utf8_lossy(&buf[..n]));
                    }
                    Ok(_) | Err(_) => {
                        // ignore read errors for this minimal example
                    }
                }

                // Minimal HTML response
                let body = "<!doctype html><html><body><h1>Hello from Rust TLS</h1></body></html>";
                let response = format!(
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: {}\r\n\r\n{}",
                    body.len(),
                    body
                );

                if let Err(e) = tls_stream.write_all(response.as_bytes()) {
                    eprintln!("write error: {e}");
                }
                // flush and shutdown (StreamOwned will flush on drop; explicitly call shutdown if you want)
                let _ = tls_stream.flush();
                // StreamOwned drop closes both TLS state and the TCP stream
            }
            Err(e) => eprintln!("accept error: {}", e),
        }
    }
}
