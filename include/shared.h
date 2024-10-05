#include <cstddef>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cassert>
#include <vector>

const size_t k_max_msg = 4096;

/**
 * @brief Prints a message to the standard error stream.
 *
 * This function takes a C-string as input and prints it to the standard error
 * stream (stderr), followed by a newline character.
 *
 * @param msg The message to be printed. It should be a null-terminated C-string.
 */
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
 * @brief Reads exactly n bytes from a file descriptor into a buffer.
 *
 * This function attempts to read n bytes from the file descriptor specified by fd
 * into the buffer pointed to by buf. It continues reading until either n bytes have
 * been read or an error occurs.
 *
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read the data into.
 * @param n The number of bytes to read.
 * @return Returns 0 on success, or -1 if an error occurs or an unexpected EOF is encountered.
 */
static int32_t read_full(int fd, char *buf, size_t n)
{
    // Read n bytes from fd into buffer buf
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // error or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv; // decrease remaining bytes
        buf += rv;       // add to buffer
    }

    return 0;
}

/**
 * @brief Writes the entire buffer to the specified file descriptor.
 *
 * This function attempts to write the entire buffer `buf` of size `n` to the file
 * descriptor `fd`. It continues writing until all bytes have been written or an
 * error occurs.
 *
 * @param fd The file descriptor to write to.
 * @param buf The buffer containing the data to write.
 * @param n The number of bytes to write from the buffer.
 * @return Returns 0 on success, or -1 if an error occurs during writing.
 */
static int32_t write_all(int fd, const char *buf, size_t n)
{
    // Write n bytes from buffer buf to file descriptor fd
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv; // decrease remaining bytes
        buf += rv;       // add to buffer
    }
    return 0;
}

enum
{
    STATE_REQ = 0, // Reading Request
    STATE_RES = 1, // Sending Response
    STATE_END = 2, // End - mark the connection for deletion
};

/**
 * @struct Conn
 * @brief Represents a connection with associated buffers and state.
 *
 * This structure is used to manage a connection, including its file descriptor,
 * state, and read/write buffers.
 *
 * @var Conn::fd
 * File descriptor for the connection. Initialized to -1.
 *
 * @var Conn::state
 * Current state of the connection. Can be either STATE_REQ or STATE_RES.
 *
 * @var Conn::rbuf_size
 * Size of the read buffer.
 *
 * @var Conn::rbuf
 * Buffer for reading data. The size is 4 + k_max_msg.
 *
 * @var Conn::wbuf_size
 * Size of the write buffer.
 *
 * @var Conn::wbuf_sent
 * Amount of data sent from the write buffer.
 *
 * @var Conn::wbuf
 * Buffer for writing data. The size is 4 + k_max_msg.
 */
struct Conn
{
    int fd = -1;
    uint32_t state = 0; // either STATE_REQ or STATE_RES

    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];

    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

/**
 * @brief Sets the file descriptor to non-blocking mode.
 *
 * This function modifies the file status flags of the specified file descriptor
 * to include the O_NONBLOCK flag, which sets the file descriptor to non-blocking mode.
 *
 * @param fd The file descriptor to be set to non-blocking mode.
 *
 * @note If an error occurs during the operation, the function will call `die` with an error message.
 */
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

/**
 * @brief Inserts a connection into the fd_to_conn vector at the index specified by the connection's file descriptor.
 *
 * This function ensures that the fd_to_conn vector is resized if necessary to accommodate the connection at the
 * specified file descriptor index. If the vector is not large enough, it will be resized to be one element larger
 * than the file descriptor value.
 *
 * @param fd_to_conn A reference to a vector of pointers to Conn objects, representing the mapping from file descriptors to connections.
 * @param conn A pointer to the Conn object to be inserted into the vector.
 */
static void conn_put(std::vector<Conn *> &fd_to_conn, struct Conn *conn) {
    if (fd_to_conn.size() <= (size_t)conn->fd) {
        fd_to_conn.resize(conn->fd + 1);
    }
    fd_to_conn[conn->fd] = conn;
}

/**
 * @brief Accepts a new connection and initializes a Conn structure.
 *
 * This function accepts a new connection on the given file descriptor, sets the
 * new connection to non-blocking mode, allocates and initializes a Conn structure,
 * and adds it to the provided vector of connections.
 *
 * @param fd2conn A reference to a vector of pointers to Conn structures, representing
 *                the current connections.
 * @param fd The file descriptor on which to accept the new connection.
 * @return int32_t Returns 0 on success, or -1 on error.
 */
static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}
