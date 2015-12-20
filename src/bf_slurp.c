#include <stddef.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bf_slurp.h>

char *slurp(const char* filename) {
    struct stat statbuf;
    int fd = -1;
    off_t filesize;
    char *memory;

    fd = open(filename, O_RDONLY);

    /* Open failed. */
    if (fd < 0) {
        return NULL;
    }

    /* Get file size, if available. */
    if (fstat(fd, &statbuf) < 0) {
        close(fd);
        return NULL;
    }

    filesize = statbuf.st_size;

    /* Check if there's actual content. */
    if (filesize < 1) {
        close(fd);
        return NULL;
    }

    /* Map the file and immediately close it. */
    memory = mmap(
            NULL,
            filesize + 1,           /* Need to write a null-terminator,
                                       one byte after the entire file. */
            PROT_READ|PROT_WRITE,   /* Need to write the zero-terminator. */
            MAP_PRIVATE,            /* On OS X, can't use MAP_SHARED with a
                                       read-only file descriptor, so we must
                                       use MAP_PRIVATE. */
            fd,
            0);
    close(fd);

    /* Could not map file. */
    if (memory == MAP_FAILED) {
        return NULL;
    }

    /* TODO: what if there are NULs in the file? Even though that's dumb? */

    /* Ensure the last byte is a zero-terminator. */
    /* Go **OUTSIDE** the buffer, and replace it with a null-terminator. */
    memory[filesize] = '\0';

    return memory;
}

bool unslurp(char *memory) {
    if (munmap(memory, 1) == 0) {
        return true;
    }

    return false;
}
