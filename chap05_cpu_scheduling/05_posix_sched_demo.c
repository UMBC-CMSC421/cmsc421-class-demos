/*
 * Chapter 5: CPU Scheduling
 * Demo 05: POSIX scheduler inspection
 *
 * What this demo shows:
 * - Every Linux thread/process has a scheduling policy and priority.
 * - Many policy changes require privileges, so this demo only *inspects* by default.
 */
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *policy_name(int policy)
{
    switch (policy) {
        case SCHED_OTHER: return "SCHED_OTHER";
#ifdef SCHED_FIFO
        case SCHED_FIFO:  return "SCHED_FIFO";
#endif
#ifdef SCHED_RR
        case SCHED_RR:    return "SCHED_RR";
#endif
        default:          return "UNKNOWN";
    }
}

int main(void)
{
    int policy = sched_getscheduler(0);
    struct sched_param param;

    if (policy < 0) {
        perror("sched_getscheduler");
        return 1;
    }

    if (sched_getparam(0, &param) < 0) {
        perror("sched_getparam");
        return 1;
    }

    printf("PID=%ld\n", (long)getpid());
    printf("Current scheduling policy : %s\n", policy_name(policy));
    printf("Current scheduling priority: %d\n", param.sched_priority);
    printf("Priority range for SCHED_OTHER is implementation-defined; Linux usually ignores numeric priority there.\n");

    return 0;
}
