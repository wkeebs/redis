#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

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

int main()
{
    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    // Configure the socket
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);                   // HTONS = Host to Network Short
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // NTOHL = Network to Host Long

    // Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0)
    {
        die("connect");
    }

    // Send a message
    char msg[] = "hello";
    if (write(fd, msg, strlen(msg)) < 0)
    {
        die("write()");
    }

    // Receive a message -> write into buffer
    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0)
    {
        die("read");
    }

    // Print the message from buffer
    printf("server says: %s\n", rbuf);

    // Close the socket
    close(fd);

    return 0;
}
