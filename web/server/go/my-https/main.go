package main

import (
	"crypto/tls"
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

func getTlsListener(port string) (net.Listener, error) {
	cert, err := tls.LoadX509KeyPair("../../cert.pem", "../../key.pem")
	if err != nil {
		log.Fatal(err)
	}

	config := &tls.Config{
		Certificates: []tls.Certificate{cert},
		InsecureSkipVerify: true,
	}

	return tls.Listen("tcp", ":" + port, config)
}

func main() {
	var port string
	var listener net.Listener
	var err error
	if len(os.Args) > 1 && os.Args[1] == "tls" {
		port = "8443"
		listener, err = getTlsListener(port)
	} else {
		port = "8000"
		listener, err = net.Listen("tcp", ":" + port)
	}

	
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("Listening on port %s\n", port)

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
