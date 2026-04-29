#include <bits/stdc++.h>
using namespace std;

struct Job {
    string pid;
    int arrival;
    int remain;
    int priority; // 0 = highest
};

struct TimelineSeg {
    int start, end;
    string pid; // "idle" for idle segments
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, quantum, steal_threshold, priority_levels, num_proc;
    if(!(cin >> n >> quantum >> steal_threshold >> priority_levels >> num_proc)){
        cerr << "Bad input. Expected: n quantum steal_threshold priority_levels num_processes\n";
        return 1;
    }
    if(n <= 0 || quantum <= 0 || priority_levels <= 0 || num_proc < 0){
        cerr << "Invalid parameters. All must be positive (num_proc >=0).\n";
        return 1;
    }

    vector<Job> all;
    all.reserve(num_proc);
    for(int i=0;i<num_proc;i++){
        Job j; cin >> j.pid >> j.arrival >> j.remain >> j.priority;
        if(j.priority < 0) j.priority = 0;
        if(j.priority >= priority_levels) j.priority = priority_levels-1;
        all.push_back(j);
    }
    // sort arrivals by arrival time then pid
    sort(all.begin(), all.end(), [](const Job&a,const Job&b){ if(a.arrival!=b.arrival) return a.arrival<b.arrival; return a.pid<b.pid; });

    // per-CPU: vector of priority queues (deque) - index 0 is highest priority
    vector<vector<deque<Job>>> queues(n, vector<deque<Job>>(priority_levels));
    vector<optional<Job>> running(n, nullopt); // currently running job on each CPU
    vector<vector<TimelineSeg>> timeline(n);
    vector<int> busy_time(n,0);
    vector<pair<string,int>> completed; // pid, finish_time

    int t = 0;
    int idx_next_arrival = 0;

    auto total_queue_length = [&](int cpu)->int{
        int s=0;
        for(int p=0;p<priority_levels;p++) s += (int)queues[cpu][p].size();
        return s;
    };

    auto total_system_work = [&](){
        int s=0;
        for(int c=0;c<n;c++) s += total_queue_length(c);
        for(int c=0;c<n;c++) if(running[c].has_value()) s += running[c]->remain;
        return s;
    };

    auto initial_dispatch = [&](const Job &job){
        // choose CPU with minimum load metric: len + running_remain/quantum
        double bestLoad = 1e100; int bestCpu = 0;
        for(int i=0;i<n;i++){
            int qlen = total_queue_length(i);
            int run_rem = running[i].has_value() ? running[i]->remain : 0;
            double metric = qlen + (double)run_rem / (double)max(1,quantum);
            if(metric < bestLoad){ bestLoad=metric; bestCpu=i; }
        }
        // push into appropriate priority queue; higher priority goes to front
        if(job.priority==0) queues[bestCpu][0].push_front(job);
        else queues[bestCpu][job.priority].push_back(job);
        return bestCpu;
    };

    auto find_victim_for_steal = [&](int me)->int{
        int victim=-1; int maxlen=-1;
        for(int i=0;i<n;i++){
            if(i==me) continue;
            int len = total_queue_length(i);
            if(len > maxlen){ maxlen=len; victim=i; }
        }
        if(victim==-1) return -1; // no other CPUs
        if(maxlen >= steal_threshold) return victim;
        return -1; // threshold not met
    };

    auto steal_from = [&](int victim)->optional<Job>{
        // steal from lowest-priority non-empty queue, take back (oldest)
        for(int p=priority_levels-1;p>=0;p--){
            if(!queues[victim][p].empty()){
                Job j = queues[victim][p].back(); queues[victim][p].pop_back();
                return j;
            }
        }
        return nullopt;
    };

    // Simulation loop: advance in quantum steps until all work done
    int max_steps = 1000000; int steps=0;
    while((idx_next_arrival < num_proc) || (total_system_work()>0) ){ 
        // admit arrivals at current time
        while(idx_next_arrival < num_proc && all[idx_next_arrival].arrival <= t){
            const Job &j = all[idx_next_arrival];
            int assigned = initial_dispatch(j);
            (void)assigned; // we could log
            idx_next_arrival++;
        }

        // for each CPU, if not running, try to pick a job from local queues; else if idle try to steal
        for(int c=0;c<n;c++){
            if(!running[c].has_value()){
                // pick highest priority non-empty
                bool found=false;
                for(int p=0;p<priority_levels;p++){
                    if(!queues[c][p].empty()){
                        Job j = queues[c][p].front(); queues[c][p].pop_front();
                        running[c] = j; found=true; break;
                    }
                }
                if(!found){
                    // try to steal if threshold satisfied
                    int victim = find_victim_for_steal(c);
                    if(victim!=-1){
                        auto stolen = steal_from(victim);
                        if(stolen.has_value()) running[c] = stolen.value();
                    }
                }
            }
        }

        // Run one quantum on each running job (if any)
        int advance = quantum; // we simulate in chunks of 'quantum'
        // But we should ensure we don't overshoot next arrival — but it's acceptable to run whole quantum; arrivals only affect dispatch next tick.

        // record timeline and update remain
        for(int c=0;c<n;c++){
            if(running[c].has_value()){
                Job &job = running[c].value();
                int run_for = min(advance, job.remain);
                timeline[c].push_back({t, t+run_for, job.pid});
                busy_time[c] += run_for;
                job.remain -= run_for;
                if(job.remain <= 0){
                    completed.push_back({job.pid, t+run_for});
                    running[c] = nullopt;
                } else {
                    // time slice expired -> requeue respecting priority (higher priority front)
                    Job newjob = job;
                    // arrival for requeued job is current time (not used further here)
                    if(newjob.priority==0) queues[c][0].push_back(newjob); // Push to back of the queue
                    else queues[c][newjob.priority].push_back(newjob);
                    running[c] = nullopt;
                }
            } else {
                // idle segment
                timeline[c].push_back({t, t+advance, string("idle")});
            }
        }

        t += advance;
        steps++; if(steps > max_steps){ cerr << "Reached max steps, aborting (possible bug or too-large sim)\n"; break; }
    }

    // Report
    int total_time = max(1, t);
    cout << "Simulation finished. Total simulated time: " << total_time << "\n\n";
    cout << "Per-CPU summary:\n";
    for(int c=0;c<n;c++){
        double util = 100.0 * (double)busy_time[c] / (double)total_time;
        cout << "CPU"<<c<<": BusyTime="<<busy_time[c]<<" Utilization(%)="<<fixed<<setprecision(2)<<util<<" CompletedAssignedCount=";
        // count how many jobs were mostly executed on this CPU (approx by timeline)
        unordered_map<string,int> time_by_pid;
        for(auto &seg : timeline[c]) if(seg.pid!="idle") time_by_pid[seg.pid] += (seg.end - seg.start);
        int assigned=0;
        for(auto &kv: time_by_pid){
            // naive: count any pid that ran on this cpu as assigned; you could assign to cpu where it spent most time
            assigned++;
        }
        cout << assigned << "\n";
    }

    cout << "\nPer-CPU timelines (segments as [start,end,pid]):\n";
    for(int c=0;c<n;c++){
        cout << "CPU"<<c<<": ";
        for(auto &s: timeline[c]){
            cout << "["<<s.start<<","<<s.end<<","<<s.pid<<"] ";
        }
        cout << "\n";
    }

    cout << "\nCompletion order and times:\n";
    sort(completed.begin(), completed.end(), [](const pair<string,int>&a,const pair<string,int>&b){ return a.second<b.second; });
    for(auto &p: completed) cout<<p.first<<" finished at "<<p.second<<"\n";

    return 0;
}
