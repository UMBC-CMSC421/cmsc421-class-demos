/*
 * Chapter 5: CPU Scheduling
 * Demo 04: Asking Pthreads for a real-time policy
 *
 * What's going on
 *   A pthread attribute object (pthread_attr_t) holds settings for threads you are
 *   about to create, including a scheduling policy and priority. This demo asks for
 *   SCHED_FIFO at priority 10 three ways and has each new thread report the policy
 *   it actually got.
 *
 *   Step 1: contention scope. Linux only supports PTHREAD_SCOPE_SYSTEM: every
 *   thread competes with every other thread on the machine. Asking for
 *   PTHREAD_SCOPE_PROCESS fails.
 *
 *   Step 2: the trap. A new thread inherits its creator's policy by default
 *   (PTHREAD_INHERIT_SCHED). Setting SCHED_FIFO in the attribute object without
 *   changing that is silently ignored, and the thread runs as SCHED_OTHER.
 *
 *   Step 3: the fix. Set PTHREAD_EXPLICIT_SCHED too. Now the policy is used, but
 *   real-time policies need root. Without it, pthread_create() fails with EPERM.
 *
 * Build
 *   make
 *
 * Run
 *   ./04_pthread_sched_attr          as a normal user
 *   sudo ./04_pthread_sched_attr     as root
 *
 * What to look for
 *   Step 1 prints "Operation not supported" for process scope.
 *   Step 2 creates the thread, but it reports SCHED_OTHER, not SCHED_FIFO.
 *   Step 3 fails with "Operation not permitted" as a normal user. With sudo, the
 *   thread reports SCHED_FIFO priority 10.
 *
 * Other ways to see it
 *   chrt -p $$            the policy and priority of your shell
 *   chrt -m               every policy and its priority range
 */
#define _GNU_SOURCE  /* needed for PTHREAD_SCOPE_* and the scheduling attribute calls */
#include <pthread.h> /* pthread_attr_t and the pthread_attr_*() functions */
#include <sched.h>   /* SCHED_OTHER, SCHED_FIFO, SCHED_RR, struct sched_param */
#include <stdio.h>   /* printf(), fprintf() */
#include <string.h>  /* strerror() */
#include <unistd.h>  /* geteuid() */

/* Turn a policy number into its name for printing. */
static const char *policy_name(int policy)
{
    switch (policy) {
    case SCHED_OTHER:
        return "SCHED_OTHER";
    case SCHED_FIFO:
        return "SCHED_FIFO";
    case SCHED_RR:
        return "SCHED_RR";
    default:
        return "another policy";
    }
}

/* Each new thread asks the kernel which policy and priority it is really running with. */
static void *report(void *arg)
{
    int policy;
    struct sched_param param;

    (void)arg;
    pthread_getschedparam(pthread_self(), &policy, &param);
    printf("  [thread] policy %s priority %d\n", policy_name(policy), param.sched_priority);
    return NULL;
}

/* Create one thread with these attributes, wait for it, and report how it went. */
static void try_create(pthread_attr_t *attr)
{
    pthread_t thread;
    int rc = pthread_create(&thread, attr, report, NULL);

    if (rc != 0) {
        printf("  pthread_create: %s\n", strerror(rc));
        return;
    }
    pthread_join(thread, NULL);
}

int main(void)
{
    pthread_attr_t attr;
    struct sched_param param;
    int scope;
    int rc;

    printf("Running as UID %ld. SCHED_FIFO priorities run %d to %d; higher wins.\n",
           (long)geteuid(), sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));

    /* Start from the default attributes. */
    pthread_attr_init(&attr);

    /* Step 1: contention scope. */
    printf("\nStep 1: contention scope\n");
    pthread_attr_getscope(&attr, &scope);
    printf("  default scope: %s\n",
           scope == PTHREAD_SCOPE_SYSTEM ? "PTHREAD_SCOPE_SYSTEM" : "PTHREAD_SCOPE_PROCESS");
    rc = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    printf("  set PTHREAD_SCOPE_PROCESS: %s\n", rc == 0 ? "ok" : strerror(rc));

    /* Step 2: ask for SCHED_FIFO at priority 10, but leave the inherit setting alone. */
    printf("\nStep 2: SCHED_FIFO priority 10, inherit setting left at its default\n");
    param.sched_priority = 10;
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
    try_create(&attr);

    /* Step 3: the same request, now telling pthread_create() to use it. */
    printf("\nStep 3: the same request plus PTHREAD_EXPLICIT_SCHED\n");
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    try_create(&attr);

    pthread_attr_destroy(&attr);
    return 0;
}
