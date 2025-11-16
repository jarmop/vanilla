package main

import (
	"fmt"
	"net"
	"os"
	"path/filepath"
	"strings"
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

	requestSlices := strings.Split(string(buf), "\r\n\r\n")
	requestHeader := requestSlices[0]

	// body := requestSlices[1]
	// trimmedBody := strings.ReplaceAll(body, "\x00", "")

	// parseHeader(string(buf))
	requestTarget := parseHeader(requestHeader)
	relativePath := "index.html"
	if len(requestTarget) > 1 {
		relativePath = requestTarget
	}
	path := filepath.Join("../../public/", relativePath)
	// path := filepath.Join("../public/", "index.html")

	requestBody, err2 := os.ReadFile(path)
	if err2 != nil {
		fmt.Println(err2)
		return
	}

	fmt.Fprint(conn, getHeader()+string(requestBody))

}

func getHeader() string {
	return "HTTP/1.1 200 OK\r\n" +
		"Content-Type: text/html\r\n" +
		"\r\n"
}

func parseHeader(header string) string {
	rows := strings.Split(header, "\r\n")
	requestLineParts := strings.Split(rows[0], " ")
	// method := requestLineParts[0]
	requestTarget := requestLineParts[1]
	// protocol := requestLineParts[2]
	
	fields := make(map[string]string)
	for _, row := range rows[1:] {
		parts := strings.Split(row, ":")
		fields[parts[0]] = strings.TrimSpace(strings.Join(parts[1:], ":"))
	}

	fmt.Printf("Host? %s\n", fields["Host"])
	fmt.Printf("cache? %s\n\n", fields["Cache-Control"])


	fmt.Printf("%s\n\n", rows[0])

	fmt.Printf("%s\n", header)

	// fmt.Printf("%q\n", strings.ReplaceAll(header, "\x00", ""))
	fmt.Printf("Request length %d\n", len(header))

	fmt.Println("\n---------------\n")

	return requestTarget
}