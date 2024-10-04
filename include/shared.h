#include <cstddef>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cassert>

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