#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <shared.h>

/**
 * @brief Handles a single request from a client.
 *
 * This function reads a request from a client, processes it, and sends a reply.
 * The request and reply follow a simple protocol where the first 4 bytes
 * indicate the length of the message body.
 *
 * @param connfd The file descriptor for the client connection.
 * @return int32_t Returns 0 on success, or a negative error code on failure.
 *
 * The function performs the following steps:
 * 1. Reads a 4-byte header to determine the length of the incoming message.
 * 2. Reads the message body based on the length specified in the header.
 * 3. Processes the request and prints the message from the client.
 * 4. Constructs a reply message and sends it back to the client.
 *
 * Error handling:
 * - If reading the header or message body fails, an error message is logged and the error code is returned.
 * - If the message length exceeds the maximum allowed length, an error message is logged and -1 is returned.
 */
static int32_t one_request(int connfd)
{
    // read the 4 byte header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4); // read 4 bytes
    if (err)
    {
        if (errno == 0)
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
        return err;
    }

    // read request length
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // copy 4 bytes to len - assume little endian
    if (len > k_max_msg)
    {
        msg("message too long");
        return -1;
    }

    // read request body
    err = read_full(connfd, rbuf, len); // read len bytes
    if (err)
    {
        msg("read() error");
        return err;
    }

    // process request (do something)
    rbuf[len] = '\0'; // null-terminate the string
    printf("client says: %s\n", &rbuf[0]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);                   // copy 4 bytes to wbuf
    memcpy(&wbuf[4], reply, len);            // copy len bytes to wbuf
    return write_all(connfd, wbuf, 4 + len); // write 4 + len bytes
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

        while (true)
        {
            int32_t err = one_request(conn_fd);
            if (err)
            {
                break;
            }
        }
        close(conn_fd); // Close the connection after handling
    }

    return 0;
}
