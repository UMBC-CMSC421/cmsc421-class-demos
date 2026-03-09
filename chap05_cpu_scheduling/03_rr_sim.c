/*
 * Chapter 5: CPU Scheduling
 * Demo 03: Round Robin simulation
 *
 * What this demo shows:
 * - The ready queue cycles through runnable processes.
 * - A time quantum limits how long each process runs before preemption.
 */
#include <stdio.h>

struct process {
    const char *name;
    int arrival;
    int burst;
    int remaining;
    int completion;
    int turnaround;
    int waiting;
};

struct queue {
    int data[64];
    int front;
    int back;
};

static void push(struct queue *q, int value)
{
    q->data[q->back++] = value;
}

static int pop(struct queue *q)
{
    return q->data[q->front++];
}

static int empty(const struct queue *q)
{
    return q->front == q->back;
}

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
        {"P1", 0, 5, 5, 0, 0, 0},
        {"P2", 1, 4, 4, 0, 0, 0},
        {"P3", 2, 2, 2, 0, 0, 0}
    };
    const int n = (int)(sizeof(p) / sizeof(p[0]));
    const int quantum = 2;
    struct queue q = {{0}, 0, 0};
    int time = 0;
    int completed = 0;
    int added[3] = {0, 0, 0};

    printf("Round Robin Gantt Chart (q=%d):\n", quantum);

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (!added[i] && p[i].arrival <= time) {
                push(&q, i);
                added[i] = 1;
            }
        }

        if (empty(&q)) {
            time++;
            continue;
        }

        int i = pop(&q);
        int slice = (p[i].remaining < quantum) ? p[i].remaining : quantum;

        printf("| %d %s %d ", time, p[i].name, time + slice);
        time += slice;
        p[i].remaining -= slice;

        for (int j = 0; j < n; j++) {
            if (!added[j] && p[j].arrival <= time) {
                push(&q, j);
                added[j] = 1;
            }
        }

        if (p[i].remaining > 0) {
            push(&q, i);
        } else {
            p[i].completion = time;
            p[i].turnaround = p[i].completion - p[i].arrival;
            p[i].waiting = p[i].turnaround - p[i].burst;
            completed++;
        }
    }
    printf("|\n");

    print_results(p, n);
    return 0;
}
