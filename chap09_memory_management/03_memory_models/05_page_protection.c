/*
 * Chapter 9: Main Memory
 * Demo 05: Linux enforces page permissions
 *
 * What's going on
 *   Unlike the table simulations, this program asks Linux to change a real
 *   mapping's permissions. Each access runs in a child. The parent uses
 *   waitpid() to report whether the child finished or received SIGSEGV.
 *   Core dumps are disabled for these intentional protection violations.
 *
 * Build
 *   make
 * Run
 *   ./05_page_protection
 *   ./05_page_protection --step
 * What to look for
 *   Reading a read-only page works; writing it triggers SIGSEGV. Restoring
 *   write permission permits the write. A second page with PROT_NONE acts
 *   as a guard: the first page's last byte is accessible, the next is not.
 */
#define _DEFAULT_SOURCE
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "walkthrough.h"

static void require(int result, const char *operation)
{
    if (result == -1) { perror(operation); exit(EXIT_FAILURE); }
}

static bool attempt(volatile unsigned char *address, bool write_access, bool expect_fault)
{
    fflush(stdout); /* Do not duplicate buffered parent output after fork(). */
    pid_t child = fork();
    if (child == -1) { perror("fork"); return false; }
    if (child == 0) {
        /* Volatile forces this access to happen instead of being optimized out. */
        if (write_access) *address = 42;
        else { unsigned char value = *address; (void)value; }
        _exit(0);
    }
    int status;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited == -1 && errno == EINTR);
    if (waited == -1) { perror("waitpid"); return false; }
    bool fault = WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV;
    bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    puts(fault ? "Result: SIGSEGV" : success ? "Result: access succeeded" : "Result: unexpected child status");
    return expect_fault ? fault : success;
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, false);
    /* Do not create a core file when the teaching example faults on purpose. */
    struct rlimit no_core = {0,0};
    require(setrlimit(RLIMIT_CORE, &no_core), "setrlimit");
    long page_long = sysconf(_SC_PAGESIZE);
    if (page_long <= 0 || (unsigned long)page_long > SIZE_MAX / 2) {
        fputs("Cannot determine a usable page size\n", stderr); return 1;
    }
    size_t page = (size_t)page_long;
    unsigned char *mapping = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED) { perror("mmap"); return 1; }
    mapping[0] = 7;
    mapping[page - 1] = 9;
    printf("Real Linux mapping: two pages, %zu bytes per page\n", page);
    require(mprotect(mapping, page, PROT_READ), "mprotect read-only");
    bool ok = true;
    puts("\nRead the read-only page"); pause_step(opt.step);
    if (!attempt(mapping, false, false)) ok = false;
    puts("\nWrite the read-only page"); pause_step(opt.step);
    if (!attempt(mapping, true, true)) ok = false;
    require(mprotect(mapping, page, PROT_READ | PROT_WRITE), "mprotect read/write");
    puts("\nRestore write permission and write again"); pause_step(opt.step);
    if (!attempt(mapping, true, false)) ok = false;
    /* mmap() returns a page-aligned address, so the second page is aligned too. */
    require(mprotect(mapping + page, page, PROT_NONE), "mprotect guard");
    puts("\nRead the first page's last byte"); pause_step(opt.step);
    if (!attempt(mapping + page - 1, false, false)) ok = false;
    puts("\nRead the next byte: the start of the guard page"); pause_step(opt.step);
    if (!attempt(mapping + page, false, true)) ok = false;
    require(munmap(mapping, 2 * page), "munmap");
    puts(ok ? "All five permission checks matched." : "A permission check failed.");
    return ok ? 0 : 1;
}
