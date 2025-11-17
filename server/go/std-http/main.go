package main

import (
	"fmt"
	"net/http"
	"time"
)

func welcome(w http.ResponseWriter, req *http.Request) {

    fmt.Fprintf(w, "welcome\n")
}


func hello(w http.ResponseWriter, req *http.Request) {

    fmt.Fprintf(w, "hello\n")
}

func headers(w http.ResponseWriter, req *http.Request) {

    for name, headers := range req.Header {
        for _, h := range headers {
            fmt.Fprintf(w, "%v: %v\n", name, h)
        }
    }
}

func nope(w http.ResponseWriter, req *http.Request) {

	fmt.Fprintf(w, "404 page not founde\n")
}

func contextExample(w http.ResponseWriter, req * http.Request) {

	// A `context.Context` is created for each request by
	// the `net/http` machinery, and is available with
	// the `Context()` method.
	ctx := req.Context()
	fmt.Println("server: hello handler started")
	defer fmt.Println("server: hello handler ended")

	// Wait for a few seconds before sending a reply to the
	// client. This could simulate some work the server is
	// doing. While working, keep an eye on the context's
	// `Done()` channel for a signal that we should cancel
	// the work and return as soon as possible.
	select {
	case <-time.After(10 * time.Second):
		fmt.Fprintf(w, "context hello\n")
	case <-ctx.Done():
		// The context's `Err()` method returns an error
		// that explains why the `Done()` channel was
		// closed.
		err := ctx.Err()
		fmt.Println("server:", err)
		internalError := http.StatusInternalServerError
		http.Error(w, err.Error(), internalError)
	}
}

func main() {

    http.HandleFunc("/", welcome)
    http.HandleFunc("/hello", hello)
    http.HandleFunc("/headers", headers)
    http.HandleFunc("/nope", nope)
    http.HandleFunc("/context-example", contextExample)

    http.ListenAndServe(":3000", nil)
}