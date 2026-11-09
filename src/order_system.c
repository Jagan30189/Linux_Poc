#include "mq_manager.h"
#include "semaphore_manager.h"
#include "stats_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t start_process(const char *program, const char *id, const char *count)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl(program, program, id, count, (char *)NULL);
        perror(program);
        _exit(EXIT_FAILURE);
    }
    return pid;
}

int main(void)
{
    char count[] = "5";
    pid_t children[4];
    int child_count = 0;

    children[child_count++] = start_process("./producer", "1", count);
    sleep(1);
    children[child_count++] = start_process("./producer", "2", count);
    children[child_count++] = start_process("./consumer", "1", count);
    children[child_count++] = start_process("./consumer", "2", count);

    for (int index = 0; index < child_count; index++) {
        int status;
        waitpid(children[index], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
            fprintf(stderr, "child process %ld failed\n", (long)children[index]);
    }

    unlink_queue();
    cleanup_semaphores();
    stats_unlink();
    return EXIT_SUCCESS;
}
