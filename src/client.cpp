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
 * @brief Sends a query to a server and processes the response.
 *
 * This function sends a text query to a server via a file descriptor and reads the server's response.
 * It handles the communication protocol, including sending the length of the query, reading the response
 * length, and ensuring the response is within acceptable limits.
 *
 * @param fd The file descriptor for the server connection.
 * @param text The text query to send to the server.
 * @return int32_t Returns 0 on success, or a negative value on error.
 *
 * The function performs the following steps:
 * 1. Calculates the length of the query text.
 * 2. Checks if the query length exceeds the maximum allowed message length.
 * 3. Constructs a write buffer with the query length and text, and sends it to the server.
 * 4. Reads the response header from the server to get the response length.
 * 5. Checks if the response length exceeds the maximum allowed message length.
 * 6. Reads the response body from the server.
 * 7. Prints the server's response.
 */
static int32_t query(int fd, const char *text)
{
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg)
    {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); // assume little endian
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len))
    {
        return err;
    }

    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
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

    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_msg)
    {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
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

    // multiple requests
    int32_t err = query(fd, "hello1");
    if (err)
    {
        goto L_DONE;
    }
    err = query(fd, "hello2");
    if (err)
    {
        goto L_DONE;
    }
    err = query(fd, "hello3");
    if (err)
    {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;

    return 0;
}
