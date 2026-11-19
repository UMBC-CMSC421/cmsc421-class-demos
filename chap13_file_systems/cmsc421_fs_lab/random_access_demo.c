/*
 * Chapter 13: File Systems
 * Lab program: random access with lseek().
 *
 * Every open file has a file offset: the position where the next read()
 * or write() starts. Sequential access lets the offset move forward on its
 * own. Random (direct) access sets the offset first with lseek(), then
 * reads from exactly that byte.
 *
 * The program writes the ten digits 0 to 9 to numbers.txt, then reads
 * back the byte at offset 0 and the byte at offset 5.
 *
 * Build: make   (builds random_access_demo)
 * Run:   ./random_access_demo
 */

#include <stdio.h>      /* printf, perror */
#include <unistd.h>     /* write, read, lseek, close */
#include <fcntl.h>      /* open and its O_ flags */
#include <stdlib.h>

int main(void) {
    /* Step 1: open numbers.txt for reading and writing.
     * O_CREAT makes the file if it does not exist, O_TRUNC empties it if
     * it does, and 0644 sets the permissions of a new file (rw-r--r--).
     * open() returns a file descriptor, a small integer, or -1 on error. */
    int fd = open("numbers.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Step 2: write the 10 bytes "0123456789".
     * The byte at offset N is the character for digit N. After this write
     * the file offset sits at 10, the end of the file. */
    const char *data = "0123456789";
    if (write(fd, data, 10) != 10) {
        perror("write");
        close(fd);
        return 1;
    }

    char c;     /* holds the one byte we read each time */


    /* Step 3: move the offset back to byte 0.
     * SEEK_SET means "count from the start of the file". Without this
     * seek, the read below would start at offset 10 and find nothing. */
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    /* Step 4: read one byte. read() also moves the offset forward by 1. */
    if (read(fd, &c, 1) != 1) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("First byte: %c\n", c);      /* prints 0 */

    /* Step 5: jump straight to offset 5. This is random access: we skip
     * bytes 1 to 4 without reading them. */
    if (lseek(fd, 5, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    /* Step 6: read the byte at offset 5. */
    if (read(fd, &c, 1) != 1) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("Byte at offset 5: %c\n", c);    /* prints 5 */

    /* Step 7: close the file descriptor. numbers.txt stays on disk;
     * look at it afterward with cat numbers.txt or od -c numbers.txt. */
    close(fd);
    return 0;
}
