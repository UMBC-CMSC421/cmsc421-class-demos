/*
 * Chapter 7: Deadlocks
 * Demo 03: Banker's Algorithm safety check
 *
 * What this demo shows:
 * - Need = Max - Allocation
 * - A state is safe if some order lets every process finish
 *
 * This uses the classic 5-process, 3-resource example.
 */
#include <stdio.h>

enum {
    PROCESS_COUNT = 5,
    RESOURCE_COUNT = 3
};

static void print_matrix(const char *title, int rows, int cols, const int m[rows][cols])
{
    printf("%s\n", title);
    for (int i = 0; i < rows; i++) {
        printf("P%d: ", i);
        for (int j = 0; j < cols; j++) {
            printf("%d ", m[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(void)
{
    int allocation[PROCESS_COUNT][RESOURCE_COUNT] = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };
    int max[PROCESS_COUNT][RESOURCE_COUNT] = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };
    int need[PROCESS_COUNT][RESOURCE_COUNT];
    int available[RESOURCE_COUNT] = {3, 3, 2};
    int work[RESOURCE_COUNT];
    int finish[PROCESS_COUNT] = {0, 0, 0, 0, 0};
    int safe_sequence[PROCESS_COUNT];
    int safe_count = 0;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        for (int j = 0; j < RESOURCE_COUNT; j++) {
            need[i][j] = max[i][j] - allocation[i][j];
        }
    }

    print_matrix("Allocation", PROCESS_COUNT, RESOURCE_COUNT, allocation);
    print_matrix("Max", PROCESS_COUNT, RESOURCE_COUNT, max);
    print_matrix("Need", PROCESS_COUNT, RESOURCE_COUNT, need);

    for (int j = 0; j < RESOURCE_COUNT; j++) {
        work[j] = available[j];
    }

    while (safe_count < PROCESS_COUNT) {
        int progress = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            int can_finish = !finish[i];

            for (int j = 0; j < RESOURCE_COUNT && can_finish; j++) {
                if (need[i][j] > work[j]) {
                    can_finish = 0;
                }
            }

            if (can_finish) {
                printf("P%d can finish; work becomes ", i);
                for (int j = 0; j < RESOURCE_COUNT; j++) {
                    work[j] += allocation[i][j];
                    printf("%d ", work[j]);
                }
                printf("\n");

                finish[i] = 1;
                safe_sequence[safe_count++] = i;
                progress = 1;
            }
        }

        if (!progress) {
            break;
        }
    }

    if (safe_count == PROCESS_COUNT) {
        printf("\nSafe sequence: ");
        for (int i = 0; i < PROCESS_COUNT; i++) {
            printf("P%d%s", safe_sequence[i], (i + 1 == PROCESS_COUNT) ? "\n" : " -> ");
        }
    } else {
        printf("\nSystem is not in a safe state.\n");
    }

    return 0;
}
