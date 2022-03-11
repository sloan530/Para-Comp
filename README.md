# Parallel_Computing
Spring 2022 COMP137 Parallel Computing

---

## Week 1

First Example Lab
- understanding relationship between p(number of cores) and n(number of values)
- core 0 adding all results vs. tree structured addition

---

## Week 3

Assignment 1

---

## Week 4

Chapter2Lab
- Estimating PI
- Parallelize the algorithm
Lab Pthreads Basics
- Using Busy-wait to resolve nondeterminism

---

## Week 5

Lab BusyWait and Timing
- Finding speedup of version 1 and 2
- Understanding why version 2 is faster
  - It is because version 1 has busy-wait inside for-loop requiring CPU to check the while loop condition every iteration.
Lab Mutex
- Finding scalability
- Weekly scalable: when n and p increase in same ratio, the efficiency does not decrease
- Strongly scalable: when n is fixed and p is increasing, the efficiency does not decrease

---

## Week 6

Assignment Pthreads
- Parallelizing the serial program using Pthreads
- fixing the race condition by locking the critical section
Lab Barrier Timing
- Experimenting the efficiency with 4 different barrier algorithms
  - pthread barrier
  - busy-wait and mutex
  - condition variable and mutex
  - semaphore

---

## Week 7

Parallelize serial program building histogram
Midterm
- Parallelizing serial program that calculates natural logarithm of 2





