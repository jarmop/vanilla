package main

import (
	"crypto/tls"
	"fmt"
	"log"
	"net"
	"os"
	"path/filepath"
	"strings"
)

func getTlsListener(port string) (net.Listener, error) {
	cert, err := tls.LoadX509KeyPair("../../cert.pem", "../../key.pem")
	if err != nil {
		return nil, err
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
		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	defer conn.Close()
	defer fmt.Print("\n---------------\n\n")

	buf := make([]byte, 4096)
	_, err := conn.Read(buf)
	if err != nil {
		log.Println("Read error:", err)
		return
	}

	requestSlices := strings.Split(string(buf), "\r\n\r\n")
	requestHeader := requestSlices[0]

	// body := requestSlices[1]
	// fmt.Printf("ORIGINAL BODY\n|%q|\n\n", body)
	// bodyTrimmed := strings.ReplaceAll(body, "\x00", "")
	// bodyTrimmed := strings.TrimSpace(body)
	// bodyTrimmed := strings.Trim(body, "\x00")
	// fmt.Printf("TRIMMED BODY\n|%q|\n\n", bodyTrimmed)

	// parseHeader(string(buf))
	requestTarget := parseHeader(requestHeader)
	relativePath := "index.html"
	if len(requestTarget) > 1 {
		relativePath = requestTarget
	}
	path := filepath.Join("../../public/", relativePath)

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
	requestLine := rows[0]
	// fieldsSection := rows[1:]
	requestLineParts := strings.Split(requestLine, " ")
	// method := requestLineParts[0]
	requestTarget := requestLineParts[1]
	// protocol := requestLineParts[2]
	
	// fields := make(map[string]string)
	// for _, row := range fieldsSection {
	// 	parts := strings.Split(row, ":")
	// 	fields[parts[0]] = strings.TrimSpace(strings.Join(parts[1:], ":"))
	// }

	fmt.Printf("REQUEST LINE\n%s\n\n", requestLine)
	// fmt.Print("FIELDS\n")
	// for _,key := range slices.Sorted(maps.Keys(fields)) {
	// 	fmt.Printf("%s: %s\n", key, fields[key])
	// }
	// fmt.Print("\n")
	// fmt.Printf("ORIGINAL HEADER\n%s\n\n", header)
	// fmt.Printf("Request length %d\n\n", len(header))

	return requestTarget
}