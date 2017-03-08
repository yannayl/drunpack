#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void foo() {return;}

int main(void) {
    void *mapped = MAP_FAILED;
    void *pageend = NULL;
    unsigned long pagesize = sysconf(_SC_PAGESIZE);

    mapped = mmap(NULL, pagesize * 2, PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == mapped) {
        return 0;
    }

    // check where foo's page ends
    // if foo overflows the page boundary we are kinda screwed...
    pageend =(void *) (((((unsigned long)foo) / pagesize) + 1) * pagesize);

    memcpy(mapped, foo, pageend - (void *)foo);
    mprotect(mapped, pagesize, PROT_EXEC);

    // call the copied function
    ((void (*)()) mapped)();
    return 0;
}
