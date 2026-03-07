#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8000

int main(void)
{
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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(main_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(main_socket, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    const char *response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "\r\n"
                          "Hello World!"
                          "<script>console.log(\"Hello!\")</script>";

    while (1) {
        int conn_socket = accept(main_socket, NULL, NULL);
        if (conn_socket < 0) {
            perror("accept");
            continue;
        }

        char buf[1024];
        ssize_t n = read(conn_socket, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("Received: %s\n", buf);
            write(conn_socket, response, strlen(response));
        }

        close(conn_socket);
    }

    return EXIT_SUCCESS;
}
