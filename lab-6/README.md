# Lab-6 Description

Modify the `void enqueue(register struct proc *rp)` function in [./proc.c](./proc.c#1624) (line 1624) to organize the process queue taking into consideration the deadline of the process.


Modify the enqueue function on `/usr/src/kernel/proc.c` so that it enqueues processes in ascending order based on the deadline parameter.

A deadline value of 0 means that the process does not have a deadline and thus needs to be enqueued at the end of correct queue based on its priority.

## Explanation

Implement an Early Deadline First (EDF) process enqueueing algorithm

One of the steps for this is to modify the queuing function of the proc.c file. This function creates the order of the ready queue so that the processes run in the desired order.

### Example

if this function is invoked with the following order of values of:
```
rp-> deadline:

rp-> deadline=6

rp-> deadline=0

rp-> deadline=5

rp-> deadline=8

rp-> deadline=7

rp-> deadline=9
```

After aplying your algorithm, the rdy queue should be in descending order but with  the edge case "deadline value of 0 means that the process does not have a deadline and thus needs to be enqueued at the end of correct queue based on its priority": **[6,7,8,9,0]**