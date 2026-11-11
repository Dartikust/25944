#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

static int attempt(const char *phase, const char *path)
{
    FILE *file;
    struct stat info;
    printf("%s: real=%ju effective=%ju\n", phase,
           (uintmax_t)getuid(), (uintmax_t)geteuid());
    if (fflush(stdout) == EOF) {
        perror("stdout");
        return 1;
    }
    /* Reject devices/directories/FIFOs; the experiment uses a stable data file. */
    if (stat(path, &info) == 0 && !S_ISREG(info.st_mode)) {
        fputs("Expected a regular data file\n", stderr);
        printf("%s: open FAILED\n", phase);
        return 1;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        printf("%s: open FAILED\n", phase);
        return 1;
    }
    if (fclose(file) == EOF) {
        perror("fclose");
        return 1;
    }
    printf("%s: open OK\n", phase);
    return 0;
}

int main(int argc, char **argv)
{
    int result;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s data-file\n", argv[0]);
        return 1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal SIGPIPE");
        return 1;
    }
    result = attempt("before", argv[1]);
    /* Drop the effective identity to the caller's real identity. */
    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }
    if (geteuid() != getuid()) {
        fputs("UIDs still differ after setuid\n", stderr);
        return 1;
    }
    result |= attempt("after", argv[1]);
    if (fflush(stdout) == EOF) {
        perror("stdout");
        return 1;
    }
    /* A denied open is an expected experiment, but remains an error code. */
    return result;
}
