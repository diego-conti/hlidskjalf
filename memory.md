Multithreading

Threads are symmetric, but each has its own memory limit, which can change. Initially, *nthreads*-1 are initiated with the lowest memory limit, and one large thread with the rest of the memory.

At each moment there is a well defined lowest effective memory limit, which is some initial constant at the beginning, and increases to the lowest memory limit of bad computations when all packed/unpacked computations are finished. Any thread which uses more memory than twice the lowest effective limit is considered a *large* thread.

Since the lowest effective limit is non-monotonous, there can be more than one large thread at a time.

As long as there are computations (packed or unpacked), the following makes sense:

Each thread
-	requests computations using add_computations_to_do. This could entail unpacking or (for the large thread) recovering bad computations. The large thread should not start too many bad computations, to avoid situations where all computations are performed by a single thread.
- performs computations.
- if halted, mark the offending computation as "bad", then resumes.

At some point, computations terminate, so the only thing to do is increase the memory limit and recover the bad. Then the large thread must release some memory in order to keep the maximum number of threads running.

Each thread
-	requests computations using add_computations_to_do. This means recovering bad computations with a lower memory limit. The large thread must not take too many bad computations, since it has to release some memory
- if it gets no computations, the thread stops, asking for more memory.
- performs computations.
- if halted, mark the offending computation as "bad"

Finally, there is not enough memory in order to allocate the maximum number of threads. There is no large thread anymore. One thread may still be larger than the others because of non-exact division, i.e. if there are 16GB and the memory limit is 5GB two get 5 and one gets 6.

Each thread
-	requests computations using add_computations_to_do. This means recovering bad computations with a lower memory limit.
- if it gets no computations, the thread stops, asking for more memory.
- performs computations.
- if halted, mark the offending computation as "bad"


Proposal:
large threads take only one computation at a time when computations.size()=0 [alternative: only one bad computation at a time]
Large threads release their memory and reallocate when computations.size()=0
non-large threads only release their memory when they have no computations to do.
