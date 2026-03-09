# Deadlock Conditions

A deadlock requires all four of the following:

1. **Mutual exclusion**  
   At least one resource is non-shareable.

2. **Hold and wait**  
   A process holds one resource while waiting for another.

3. **No preemption**  
   Resources cannot be forcibly taken away.

4. **Circular wait**  
   A cycle of waiting processes exists.

## Teaching Prompt

Ask students:

- Which of the four conditions does a mutex satisfy?
- Which condition is broken by forcing a global lock order?
- Why does the Banker's Algorithm reason about *safe states* instead of only current waits?
