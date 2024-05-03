A simple toy example system that tests deadlock detection of the join() operation.

Correct output:
```
Deadlocked PD: starting
Deadlocked PD: libmicrokitco started
Deadlocked PD: cothreads spawned
Deadlocked PD: root: joining co1
Deadlocked PD: co1: joining co2
Deadlocked PD: co2: joining co3
Deadlocked PD: co3: joining co1
Deadlocked PD: co3: DEADLOCK DETECTED...OK
Deadlocked PD: root: finished ok
Many to one PD: starting
Many to one PD: libmicrokitco started
Many to one PD: cothreads 1 2 3 4 spawned
Many to one PD: root: joining co1
Many to one PD: co1: joining co4
Many to one PD: co2: joining co4
Many to one PD: co3: joining co4
Many to one PD: co4 returning magic
Many to one PD: co1: awaken with correct magic
Many to one PD: co2: awaken with correct magic
Many to one PD: co3: awaken with correct magic
Many to one PD: root: joining co2
Many to one PD: root: joining co3
Many to one PD: root: all magic CORRECT, finished OK
Deadlock-free PD: starting
Deadlock-free PD: libmicrokitco started
Deadlock-free PD: cothreads spawned
Deadlock-free PD: root: joining co1
Deadlock-free PD: co1: joining co2
Deadlock-free PD: co2: joining co3
Deadlock-free PD: co3: return magic 1
Deadlock-free PD: co2: magic 1 correct, return magic 2
Deadlock-free PD: co1: magic 2 correct, returning magic 3
Deadlock-free PD: root: magic 3 correct
```