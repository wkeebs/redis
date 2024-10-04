#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>  // Include this header for sockaddr_in
#include <unistd.h>

/**
 * @brief Terminates the program with an error message.
 *
 * This function prints the provided error message using perror and then
 * terminates the program with an exit status of EXIT_FAILURE.
 *
 * @param msg The error message to be printed.
 */
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


/**
 * @brief Dummy processing function.
 *
 * This function reads data from a given connection file descriptor, prints the received message,
 * and sends a response back to the client.
 *
 * @param connfd The connection file descriptor from which to read and write data.
 */
static void do_something(int connfd) {

    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

int main() {
    std::cout << "Hello, Redis Server!" << std::endl;

    // obtain socket handle
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    // configure socket
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind to wildcard address 0.0.0.0:1234
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234); // Network To Host Short
    addr.sin_addr.s_addr = ntohl(0); // Network To Host Long - wildcard address 0.0.0.0
    int bind_rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (bind_rv != 0) {
        // bind failed
        die("bind()");
    }

    // listen for incoming connections
    int listen_rv = listen(fd, SOMAXCONN); // SOMAXCONN = max size of queue (128)
    if (listen_rv != 0) {
        // listen failed
        die("listen()");
    }

    // accept incoming connections
    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int conn_fd = accept(fd, (sockaddr *)&client_addr, &addrlen);
        if (conn_fd != 0) {
            continue; // error
        }
        do_something(conn_fd);
        close(conn_fd);
    }

    return 0;
}