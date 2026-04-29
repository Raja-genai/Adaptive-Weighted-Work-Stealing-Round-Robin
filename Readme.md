AWS-RR N-CPU Scheduler
Description
This program simulates an Adaptive Weighted Work-Stealing Round-Robin (AWS-RR) scheduler for N CPUs. It uses multi-level priority queues and supports work-stealing when CPUs are idle.

How to Compile and Run
1. Compile the Program
Use the following command to compile the program with g++:

g++ -std=c++17 -o aws_rr_n_cpu_scheduler aws_rr_n_cpu_scheduler.cpp
2. Run the Program
Run the compiled program with input redirection:

./aws_rr_n_cpu_scheduler < input.txt
3. Input Format
The input file should follow this format:

n quantum steal_threshold priority_levels num_processes
pid arrival burst priority
n: Number of CPUs
quantum: Time slice for round-robin scheduling
steal_threshold: Minimum queue length for work-stealing
priority_levels: Number of priority levels (0 = highest priority)
num_processes: Total number of processes
Each process line contains:
pid: Process ID
arrival: Arrival time
burst: Burst time (execution time)
priority: Priority level (0 = highest)
4. Example Input File
Example content for input.txt:

4 2 3 5 6
P1 0 10 1
P2 1 5 0
P3 2 8 2
P4 3 6 1
P5 4 4 3
P6 5 7 0
5. Output
The program outputs the following:

Total simulated time
Per-CPU utilization and timelines
Completion order and times
