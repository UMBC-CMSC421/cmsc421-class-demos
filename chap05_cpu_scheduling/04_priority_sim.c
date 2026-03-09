/*
 * Chapter 5: CPU Scheduling
 * Demo 04: Priority scheduling (non-preemptive)
 *
 * Lower numeric value means higher priority.
 */
#include <stdio.h>

struct process {
    const char *name;
    int arrival;
    int burst;
    int priority;
    int completion;
    int turnaround;
    int waiting;
    int done;
};

static void print_results(const struct process *p, int n)
{
    double total_wait = 0.0;
    double total_turnaround = 0.0;

    printf("\n%-4s %-7s %-5s %-8s %-10s %-10s %-7s\n",
           "Proc", "Arrival", "Burst", "Priority", "Completion", "Turnaround", "Wait");
    for (int i = 0; i < n; i++) {
        printf("%-4s %-7d %-5d %-8d %-10d %-10d %-7d\n",
               p[i].name, p[i].arrival, p[i].burst, p[i].priority,
               p[i].completion, p[i].turnaround, p[i].waiting);
        total_wait += p[i].waiting;
        total_turnaround += p[i].turnaround;
    }

    printf("\nAverage waiting time    = %.2f\n", total_wait / n);
    printf("Average turnaround time = %.2f\n", total_turnaround / n);
}

static int choose_next(struct process *p, int n, int time)
{
    int best = -1;

    for (int i = 0; i < n; i++) {
        if (p[i].done || p[i].arrival > time) {
            continue;
        }
        if (best == -1 || p[i].priority < p[best].priority) {
            best = i;
        }
    }

    return best;
}

int main(void)
{
    struct process p[] = {
        {"P1", 0, 6, 3, 0, 0, 0, 0},
        {"P2", 1, 2, 1, 0, 0, 0, 0},
        {"P3", 2, 4, 4, 0, 0, 0, 0},
        {"P4", 3, 3, 2, 0, 0, 0, 0}
    };
    const int n = (int)(sizeof(p) / sizeof(p[0]));
    int completed = 0;
    int time = 0;

    printf("Priority Gantt Chart:\n");

    while (completed < n) {
        int next = choose_next(p, n, time);

        if (next == -1) {
            time++;
            continue;
        }

        printf("| %d %s %d ", time, p[next].name, time + p[next].burst);
        time += p[next].burst;
        p[next].completion = time;
        p[next].turnaround = p[next].completion - p[next].arrival;
        p[next].waiting = p[next].turnaround - p[next].burst;
        p[next].done = 1;
        completed++;
    }
    printf("|\n");

    print_results(p, n);
    return 0;
}
