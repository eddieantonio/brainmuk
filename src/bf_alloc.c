#include <bf_alloc.h>

#include <sys/mman.h>

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

bool free_executable_space(uint8_t *space) {
    if (munmap(space, 1) == 0) {
        return true;
    }
    return false;
}
