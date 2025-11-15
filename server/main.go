package main

import (
	"fmt"
	"net"
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

	response := "HTTP/1.1 200 OK\r\n" +
				"Content-Type: text/html\r\n" +
				"\r\n" +
				"Hello World!" +
				"<script>alert(\"Hello!\")</script>"

	fmt.Fprint(conn, response)

	fmt.Printf("Received: %s\n", buf)
}

// ln, err := net.Listen("tcp", ":8080")
// if err != nil {
// 	// handle error
// }
// for {
// 	conn, err := ln.Accept()
// 	if err != nil {
// 		// handle error
// 	}
// 	go handleConnection(conn)
// }
