// tls_server.c — Minimal TLS server in C using OpenSSL

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8443

SSL_CTX *init_ssl() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        return NULL;
    }

    // Load certificate and private key (PEM files)
    if (SSL_CTX_use_certificate_file(ctx, "../../cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "../../key.pem", SSL_FILETYPE_PEM) <= 0) {
        return NULL;
    }

    return ctx;
}

int main() {

    int main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Allow reusing the address immediately after this program is closed. 
    When a TCP socket is released, it's kept reserved for 60 seconds (TIME_WAIT),
    to make sure that delayed packets are not confusing other connections. 
    Setting SO_REUSEADDR to 1 allows binding the socket to a port that is in
    TIME_WAIT. */
    int sockopt_value = 1;
    setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt_value, sizeof(sockopt_value));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(main_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(main_socket, 1) < 0) {
        perror("listen");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    SSL_CTX *ctx = init_ssl();
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int conn_socket = accept(main_socket, NULL, NULL);
        printf("Received connection\n");
        if (conn_socket < 0) {
            perror("accept");
            continue;
        }

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, conn_socket);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(conn_socket);
            continue;
        }

        char buf[4096];
        ssize_t n = SSL_read(ssl, buf, sizeof(buf));
        if (n > 0) {
            buf[n] = '\0';
            printf("Received: %s\n", buf);

            // Minimal valid HTTP response
            const char *response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "Hello World!"
                "<script>console.log(\"Hello!\")</script>";

            SSL_write(ssl, response, strlen(response));

            // write(conn_socket, response, strlen(response));
        }
 
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(conn_socket);
    }

    exit(EXIT_SUCCESS);
}
