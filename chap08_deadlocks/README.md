# Chapter 7 — Deadlocks

Deadlocks are hard to trigger cleanly in a safe and portable way from user space, so this chapter
mixes **one live deadlock example** with **simulation programs**.

## Learning Goals

Students should be able to:

- state the four necessary conditions for deadlock
- distinguish safe and unsafe states
- trace a basic resource-allocation scenario
- compute a Need matrix
- explain how the Banker's Algorithm checks safety

## Demos

1. `01_deadlock_conditions.md` — concept summary for lecture
2. `02_deadlock_detection_sim.c` — single-instance detection style simulation
3. `03_bankers_algorithm_sim.c` — safety check for a multi-instance resource snapshot
