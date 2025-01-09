#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void info(const char *label, const char *string) {
    fprintf(stderr, "%s: %s\n", label, string);
}

void error(const char *string) {
    info("error", string);
    exit(EXIT_FAILURE);
}

void print_addr(struct sockaddr_in addr) {
    char str[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &addr.sin_addr, str, INET_ADDRSTRLEN) == NULL)
        error("invalid address");

    printf("%s:%d\n", str, ntohs(addr.sin_port));
}

void *handle_request(void *arg) {
    int client = *(int *)arg;
    char buff[BUFSIZ];

    while (recv(client, buff, BUFSIZ, MSG_DONTWAIT) > 0); // TODO: Actually read incoming requests

    if (write(client, "HTTP/1.1 200 OK\r\n\r\n<html><body style=\"font-size: 128;\">Henlo</body></html>", 74) <= 0)
        info("warning", "write failed");

    if (close(client) < 0)
        info("warning", "close failed");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        info("usage", "server <address> <port>");
        exit(EXIT_FAILURE);
    }
    int server, client, temp = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof (client_addr);
    pthread_t thread;

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
        error("invalid address");

    if ((temp = strtol(argv[2], NULL, 10)) <= 0 || temp > USHRT_MAX)
        error("invalid port");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(temp);

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket failed");

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof temp) < 0)
        error("setsockopt failed");

    if (bind(server, (struct sockaddr *)&server_addr, sizeof server_addr) < 0)
        error("bind failed");

    if (listen(server, 128) < 0)
        error("listen failed");

    printf("listening on ");
    print_addr(server_addr);

    while (1) {
        client = accept(server, (struct sockaddr*)&client_addr, &client_len);
        printf("client connected from ");
        print_addr(client_addr);

        if (client >= 0) {
            pthread_create(&thread, NULL, handle_request, (void *)&client);
            pthread_detach(thread);
        }
        else info("info", "connection failed");
    }
    return EXIT_SUCCESS;
}
