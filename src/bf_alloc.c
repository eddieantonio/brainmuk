#include <bf_alloc.h>

/* So that MAP_ANONYMOUS is available on glibc. */
#define _BSD_SOURCE
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

uint8_t *allocate_executable_space(size_t size) {
    uint8_t *memory = (uint8_t*) mmap(
            NULL,                       /* No existing address. */
            size,                       /* At LEAST this size. */
            PROT_EXEC|PROT_WRITE,       /* Writable and executable memory. */
            MAP_PRIVATE|MAP_ANONYMOUS,  /* An anonymous mapping; using private
                                           mapping is most portable. */
            -1,                         /* Using fd = -1 is most portable in
                                           conjunction with MAP_ANONYMOUS. */
            0                           /* Irrelevant for MAP_ANONYMOUS. */
    );

    if (memory == MAP_FAILED) {
        return NULL;
    }

    return memory;
}

/* TODO: fairly certain this will lead to a memory leak if the mapping is
 * larger than the page size... */

bool free_executable_space(uint8_t *space) {
    if (munmap(space, 1) == 0) {
        return true;
    }
    return false;
}
