// bankers.c: the banker's algorithm on the textbook's example (Sec. 8.6).
// Ch 8 Day 2 slides: Safety Trace: The Snapshot Is Safe; T1 Requests (1,0,2); Linux: Run the Banker.
// Five threads T0..T4, three resource types A, B, C with 10, 5, 7 instances.
//   1. Need = Max - Allocation, then the safety algorithm, printing Work at every step.
//   2. Three requests from the textbook, each checked with the resource-request algorithm:
//      T1 asks for (1,0,2), then (in that new state) T4 asks for (3,3,0) and T0 for (0,2,0).
// EXPECT: a safe sequence; T1 granted; T4 must wait (not available); T0 must wait (unsafe).
// BUILD: make            (or: gcc -O2 -Wall -Wextra bankers.c -o bankers)
// RUN:   ./bankers
#include <stdio.h>
#include <string.h>

#define N 5                      // threads
#define M 3                      // resource types

int available[M]     = {3, 3, 2};
int max[N][M]        = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
int allocation[N][M] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
int need[N][M];

static void vec(const int *v) { printf("(%d,%d,%d)", v[0], v[1], v[2]); }

static int leq(const int *a, const int *b) {             // a <= b in every position
    for (int j = 0; j < M; j++) if (a[j] > b[j]) return 0;
    return 1;
}

// Safety algorithm. Prints each step when verbose. Returns 1 if the state is safe.
static int safe(int verbose) {
    int work[M], finish[N] = {0}, order[N], count = 0;
    memcpy(work, available, sizeof work);                // step 1: Work = Available
    if (verbose) { printf("  Work = "); vec(work); printf("\n"); }
    for (int found = 1; found; ) {                       // step 2: find i with Finish false, Need <= Work
        found = 0;
        for (int i = 0; i < N; i++) {
            if (finish[i] || !leq(need[i], work)) continue;
            for (int j = 0; j < M; j++) work[j] += allocation[i][j];   // step 3: Ti finishes, returns all
            finish[i] = 1; order[count++] = i; found = 1;
            if (verbose) { printf("  T%d: Need ", i); vec(need[i]); printf(" <= Work, finishes; Work = "); vec(work); printf("\n"); }
        }
    }
    if (verbose) {                                       // step 4: safe if every Finish is true
        if (count == N) {
            printf("  safe sequence: <");
            for (int k = 0; k < N; k++) printf("T%d%s", order[k], k + 1 < N ? ", " : ">\n");
        } else {
            printf("  no thread can finish with Work = "); vec(work); printf(": unsafe\n");
        }
    }
    return count == N;
}

// Resource-request algorithm for thread t. Grants (keeps the new state) or restores the old one.
static void request(int t, int a, int b, int c) {
    int req[M] = {a, b, c};
    printf("\nT%d requests ", t); vec(req); printf("\n");
    if (!leq(req, need[t])) { printf("  error: more than T%d's maximum claim\n", t); return; }
    printf("  Request <= Need "); vec(need[t]); printf(": yes\n");
    if (!leq(req, available)) {
        printf("  Request <= Available "); vec(available); printf(": no, T%d must wait\n", t);
        return;
    }
    printf("  Request <= Available "); vec(available); printf(": yes, pretend to allocate\n");
    for (int j = 0; j < M; j++) { available[j] -= req[j]; allocation[t][j] += req[j]; need[t][j] -= req[j]; }
    if (safe(1)) {
        printf("  granted\n");
    } else {
        for (int j = 0; j < M; j++) { available[j] += req[j]; allocation[t][j] -= req[j]; need[t][j] += req[j]; }
        printf("  T%d must wait; old state restored\n", t);
    }
}

int main(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++) need[i][j] = max[i][j] - allocation[i][j];
    printf("Need = Max - Allocation\n");
    for (int i = 0; i < N; i++) { printf("  T%d: ", i); vec(need[i]); printf("\n"); }
    printf("\nIs the snapshot safe?\n");
    safe(1);
    request(1, 1, 0, 2);         // textbook: granted
    request(4, 3, 3, 0);         // textbook: not available
    request(0, 0, 2, 0);         // textbook: available, but unsafe
    return 0;
}
