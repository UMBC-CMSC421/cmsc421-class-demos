// detect.c: the deadlock-detection algorithm on the textbook's example (Sec. 8.7).
// Ch 8 Day 2 slides: Detection Example: No Deadlock; T2 Asks for One More C; Linux: Run the Detector.
// Five threads T0..T4, three resource types A, B, C with 7, 2, 6 instances.
//   1. The snapshot: every thread can finish, so no deadlock.
//   2. T2 asks for one more C. Now T1, T2, T3, T4 are deadlocked.
// Compare with bankers.c: detection uses each thread's current Request, not its Need,
// and a thread holding nothing starts with Finish = true.
// EXPECT: part 1 finishes every thread; part 2 reports T1, T2, T3, T4 deadlocked.
// BUILD: make            (or: gcc -O2 -Wall -Wextra detect.c -o detect)
// RUN:   ./detect
#include <stdio.h>
#include <string.h>

#define N 5
#define M 3

int available[M]     = {0, 0, 0};
int allocation[N][M] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 3}, {2, 1, 1}, {0, 0, 2}};
int request[N][M]    = {{0, 0, 0}, {2, 0, 2}, {0, 0, 0}, {1, 0, 0}, {0, 0, 2}};

static void vec(const int *v) { printf("(%d,%d,%d)", v[0], v[1], v[2]); }

static void detect(void) {
    int work[M], finish[N];
    memcpy(work, available, sizeof work);                // step 1: Work = Available
    for (int i = 0; i < N; i++)                          // a thread holding nothing cannot be in a cycle
        finish[i] = !(allocation[i][0] || allocation[i][1] || allocation[i][2]);
    printf("  Work = "); vec(work); printf("\n");
    for (int found = 1; found; ) {                       // step 2: Finish false and Request <= Work
        found = 0;
        for (int i = 0; i < N; i++) {
            if (finish[i]) continue;
            int ok = 1;
            for (int j = 0; j < M; j++) if (request[i][j] > work[j]) ok = 0;
            if (!ok) continue;
            for (int j = 0; j < M; j++) work[j] += allocation[i][j];   // step 3: assume Ti finishes
            finish[i] = 1; found = 1;
            printf("  T%d: Request ", i); vec(request[i]); printf(" <= Work, reclaim; Work = "); vec(work); printf("\n");
        }
    }
    int dead = 0;                                        // step 4: any Finish false means deadlock
    for (int i = 0; i < N; i++) if (!finish[i]) dead = 1;
    if (!dead) { printf("  every Finish is true: no deadlock\n"); return; }
    printf("  deadlocked:");
    for (int i = 0; i < N; i++) if (!finish[i]) printf(" T%d", i);
    printf("\n");
}

int main(void) {
    printf("Snapshot\n");
    detect();
    request[2][2] = 1;                                   // T2 asks for one more instance of C
    printf("\nT2 requests one more C: Request[T2] = "); vec(request[2]); printf("\n");
    detect();
    return 0;
}
