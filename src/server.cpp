#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

/**
 * @brief Terminates the program with an error message.
 *
 * This function prints the provided error message using perror and then
 * terminates the program with an exit status of EXIT_FAILURE.
 *
 * @param msg The error message to be printed.
 */
static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

/**
 * @brief Dummy processing function.
 *
 * This function reads data from a given connection file descriptor, prints the received message,
 * and sends a response back to the client.
 *
 * @param connfd The connection file descriptor from which to read and write data.
 */
static void do_something(int connfd)
{

    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0)
    {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

int main()
{
    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    // Configure the socket to reuse the address
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind to wildcard address 0.0.0.0:1234
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // HTONS = Host to Network Short
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket
    int bind_rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (bind_rv)
    {
        die("bind()");
    }

    // Listen for incoming connections
    int listen_rv = listen(fd, SOMAXCONN);
    if (listen_rv)
    {
        die("listen()");
    }

    // Accept and handle incoming connections
    while (true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int conn_fd = accept(fd, (sockaddr *)&client_addr, &addrlen);
        if (conn_fd < 0)
        {
            continue; // Error in accept, continue to next
        }

        // Handle the connection
        do_something(conn_fd);
        close(conn_fd); // Close the connection after handling
    }

    return 0;
}
