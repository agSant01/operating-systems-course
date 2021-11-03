"""
Bankers Algorithm is a deadlock avoidance algorithm used to allocate resources in a system 
where multiple instances of a resource type exists. 

This algorithm is not very efficient, but it accomplished the goal of ensure that the system can satisfy the 
request of multiple processes without deadlocks, as long as each process *DECLARES* with anticipation
the maximum number of instances of each resource type that it may need.

- The number requested **MUST** not exceed the total number of resources in the system.

# Required Data Structures

## Notation
"n" indicates the number of processes, and "m" indicates the number of resource types.

- Available: A vector of length "m" (all resource types). Indicates the number of available instances per resource type.
    - Available[j] = k; j => Resource Type j; k => quantity available
- Max: Matrix (n x m), Maximum demand of each process;
    - Max[i][j] = k,  Process_i needs a maximum quantity "k" from Resource_j 
- Allocation: Matrix (n x m); Current allocation at time t(x) for process P_i of resource R_j
    - Allocation[i][j] = k; k => amount allocated of resource type R_j to Process P_i
- Need: k Amount of resource instance needed for Process_i from Resource_j
"""
import queue
import time
import numpy as np
import random

random.seed(0)

AMOUNT_OF_RESOURCES = 30  # m
AMOUNT_OF_PROCESSES = 20  # n

MAX_NUMBER_OF_AVAILABLE_RESOURCES = 10

MAX_POSSIBLE_REQ_OF_PROCESS = 7
MAX_INITIAL_ALLOCATION = 3

available = [
    random.randint(2, MAX_NUMBER_OF_AVAILABLE_RESOURCES)
    for _ in range(AMOUNT_OF_RESOURCES)
]
available = np.array(available)

max_request = []
for _ in range(AMOUNT_OF_PROCESSES):
    requests_of_process = [random.randint(0, MAX_POSSIBLE_REQ_OF_PROCESS)
                           for _ in range(AMOUNT_OF_RESOURCES)]
    max_request.append(requests_of_process)
max_request = np.array(max_request)

allocation = []
for i in range(AMOUNT_OF_PROCESSES):
    init_alloc = [random.randint(0, max_request[i][j])
                  for j in range(AMOUNT_OF_RESOURCES)]
    allocation.append(init_alloc)
allocation = np.array(allocation)

need = np.subtract(max_request, allocation)

print('available\n', available)
print('max_request\n', max_request)
print('allocation\n', allocation)
print('need\n', need)

# print(max_request[0])
# print(allocation[0])
# print(need[0])

ORDER = []
FINISHED = [False for _ in range(AMOUNT_OF_PROCESSES)]

p_id_list = queue.Queue()
for pid in range(AMOUNT_OF_PROCESSES):
    p_id_list.put(pid)

print(p_id_list.qsize())

while not p_id_list.empty():
    p_id = p_id_list.get()
    print('pid:', p_id)

    if FINISHED[p_id]:
        # already finished, no need to compute
        continue

    p_resource_need = need[p_id]
    res_satisfied = True
    for res_id, res_instances_needed in enumerate(p_resource_need):
        if res_instances_needed > available[res_id]:

            print('resid:', res_id, 'defecit',
                  available[res_id] - res_instances_needed)
            res_satisfied = False
            break

    print(p_resource_need)
    print(available)
    print(res_satisfied)
    time.sleep(1)
    if res_satisfied:
        FINISHED[p_id] = True
        ORDER.append(p_id)
        available += allocation[p_id]
        print(FINISHED.count(False))
        print(ORDER)
    else:
        p_id_list.put(p_id)

# print('p_resource_need:\n', p_resource_need)
# print('Available:\n', available)
# print('allocation[p_id]:\n', allocation[p_id])

# print('Av Sum Alloc:\n', available + allocation[p_id])

print('Order:', ORDER)
