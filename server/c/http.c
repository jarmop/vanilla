#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    /* Allow reusing the address immediately after this program is closed. 
    When a TCP socket is released, it's kept reserved for 60 seconds (TIME_WAIT),
    to make sure that delayed packets are not confusing other connections. 
    Setting SO_REUSEADDR to 1 allows binding the socket to a port that is in
    TIME_WAIT. */
    int sockopt_value = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt_value, sizeof(sockopt_value));

    int port = 3000;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    fprintf(stderr, "Listening port %d\n\n", port);

    char response[1024] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "\r\n"
                          "Hello World!"
                          "<script>console.log(\"Hello!\")</script>";

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            exit(1);
        }

        char buf[1024];
        ssize_t n = read(client_fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("Received: %s\n", buf);
            write(client_fd, response, strlen(response));
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
