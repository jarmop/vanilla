package main

import (
	"fmt"
	"net"
	"os"
)

func main() {
	ln, err := net.Listen("tcp", ":3000")
	if err != nil {
		fmt.Println(err)
	}

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println(err)
		}
		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	defer conn.Close()

	buf := make([]byte, 1024)
	_, err := conn.Read(buf)
	if err != nil {
		fmt.Println(err)
		return
	}

	path := "../public/index.html"
	content, err2 := os.ReadFile(path)
	if err2 != nil {
		fmt.Println(err2)
		return
	}

	fmt.Fprint(conn, getHeader()+string(content))

	fmt.Printf("%s\n", buf)
}

func getHeader() string {
	return "HTTP/1.1 200 OK\r\n" +
		"Content-Type: text/html\r\n" +
		"\r\n"
}
