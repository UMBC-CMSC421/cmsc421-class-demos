/*
 * Chapter 7: Deadlocks
 * Demo 02: Single-instance deadlock detection style simulation
 *
 * What this demo shows:
 * - With single-instance resources, a cycle in a wait-for graph implies deadlock.
 * - We simulate a wait-for graph as an adjacency matrix.
 */
#include <stdio.h>

static int has_cycle_from(int v, int n, const int graph[n][n], int visited[n], int stack[n])
{
    visited[v] = 1;
    stack[v] = 1;

    for (int u = 0; u < n; u++) {
        if (!graph[v][u]) {
            continue;
        }
        if (!visited[u] && has_cycle_from(u, n, graph, visited, stack)) {
            return 1;
        }
        if (stack[u]) {
            return 1;
        }
    }

    stack[v] = 0;
    return 0;
}

int main(void)
{
    enum { N = 3 };
    const char *names[N] = {"P0", "P1", "P2"};
    int graph[N][N] = {
        {0, 1, 0}, /* P0 waits for P1 */
        {0, 0, 1}, /* P1 waits for P2 */
        {1, 0, 0}  /* P2 waits for P0 */
    };
    int visited[N] = {0, 0, 0};
    int stack[N] = {0, 0, 0};
    int cycle = 0;

    printf("Wait-for edges:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (graph[i][j]) {
                printf("  %s -> %s\n", names[i], names[j]);
            }
        }
    }

    for (int i = 0; i < N; i++) {
        if (!visited[i] && has_cycle_from(i, N, graph, visited, stack)) {
            cycle = 1;
            break;
        }
    }

    if (cycle) {
        printf("\nCycle detected: deadlock exists in this single-instance model.\n");
    } else {
        printf("\nNo cycle detected.\n");
    }

    return 0;
}
