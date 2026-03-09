/*
 * Chapter 5: CPU Scheduling
 * Demo 01: FCFS simulation
 *
 * What this demo shows:
 * - Processes are scheduled in order of arrival.
 * - Once a process starts, it runs until completion.
 */

#include <stdio.h>

struct process {
    const char *name;
    int arrival;
    int burst;
    int completion;
    int turnaround;
    int waiting;
    int done;
};

static void print_results(const struct process *p, int n)
{
    double total_wait = 0.0;
    double total_turnaround = 0.0;

    printf("\n%-4s %-7s %-5s %-10s %-10s %-7s\n",
           "Proc", "Arrival", "Burst", "Completion", "Turnaround", "Wait");
    for (int i = 0; i < n; i++) {
        printf("%-4s %-7d %-5d %-10d %-10d %-7d\n",
               p[i].name, p[i].arrival, p[i].burst,
               p[i].completion, p[i].turnaround, p[i].waiting);
        total_wait += p[i].waiting;
        total_turnaround += p[i].turnaround;
    }

    printf("\nAverage waiting time    = %.2f\n", total_wait / n);
    printf("Average turnaround time = %.2f\n", total_turnaround / n);
}

int main(void)
{
    struct process p[] = {
        {"P1", 0, 5, 0, 0, 0, 0},
        {"P2", 1, 3, 0, 0, 0, 0},
        {"P3", 2, 8, 0, 0, 0, 0},
        {"P4", 3, 6, 0, 0, 0, 0}
    };
    const int n = (int)(sizeof(p) / sizeof(p[0]));
    int time = 0;

    printf("FCFS Gantt Chart:\n");

    for (int i = 0; i < n; i++) {
        if (time < p[i].arrival) {
            time = p[i].arrival;
        }

        printf("| %d %s %d ", time, p[i].name, time + p[i].burst);
        time += p[i].burst;
        p[i].completion = time;
        p[i].turnaround = p[i].completion - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
    }
    printf("|\n");

    print_results(p, n);
    return 0;
}
