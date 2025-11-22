package main

import (
	"crypto/tls"
	"fmt"
	"io"
	"log"
	"net"
)

func main() {
	cert, err := tls.LoadX509KeyPair("../../cert.pem", "../../key.pem")
	if err != nil {
		log.Fatal(err)
	}

	config := &tls.Config{
		Certificates: []tls.Certificate{cert},
		// Required for self-signed certs so Chrome doesn't immediately reject handshake
		InsecureSkipVerify: true,
	}

	listener, err := tls.Listen("tcp", ":8443", config)
	if err != nil {
		log.Fatal(err)
	}
	defer listener.Close()

	fmt.Println("Listening on https://localhost:8443")

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println("Accept error:", err)
			continue
		}

		go handle(conn)
	}
}

func handle(conn net.Conn) {
	defer conn.Close()

	buf := make([]byte, 4096)
	_, err := conn.Read(buf)
	if err != nil && err != io.EOF {
		log.Println("Read error:", err)
		return
	}

	// Minimal valid HTTP 1.1 response
	resp := "HTTP/1.1 200 OK\r\n" +
		"Content-Type: text/plain\r\n" +
		"Content-Length: 13\r\n" +
		"\r\n" +
		"Hello, World!"

	conn.Write([]byte(resp))
}
