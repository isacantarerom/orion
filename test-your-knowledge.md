# 


### Bash: 

top       → what is my CPU doing RIGHT NOW?
dmesg     → what did the kernel see? (hardware, crashes, OOM)
tcpdump   → what packets are actually on the wire?
strace    → what is this process actually doing?
vmstat    → CPU + memory over time
journalctl→ what happened before the crash?



---


### Top 

> What it's for: See which processes are eating CPU and memory right now.
You type top and see this:>

```
top - 14:32:01 up 3 days
Tasks: 182 total, 1 running, 181 sleeping
%Cpu(s): 94.3 us,  3.1 sy,  0.0 ni,  1.2 id

  PID  USER    %CPU  %MEM  COMMAND
 4821  ci_run  91.2   2.1  orion_tests
 1204  root     2.1   0.3  systemd
  892  root     0.8   0.1  sshd
```


**"I'd reach for [top] when [I need to check the CPU and memory usage a process is using/consuming] because [it shows me the running process and the %s it is using] that way I can prove/disprove the hypotesis of a process overflowing a CPU."

  ---

  ### dmesg

  > What it's for: The kernel's diary. Every time hardware does something notable, the kernel writes it here — crashes, memory errors, device failures, processes getting killed.
    You type dmesg | tail -20 and see this:

  ```
[142031.441] eth0: renamed from veth3a2f
[142035.112] TCP: request_sock_TCP: Possible SYN flooding on port 8080
[142089.331] Out of memory: Killed process 4821 (orion_tests) score 892
[142089.332] orion_tests: page allocation failure
[142089.334] eth0: link is not ready
[142102.981] systemd[1]: orion_tests.service: Main process exited
  ```

** "I'd reach for [dmesg] when [a process crashed unexpectedly or hardware is behaving strangely], because [it shows me the kernel's log of hardware events, OOM kills, and device errors — things that happen below the application layer that no application log would ever capture]."
---

  ### tcpdump

> What it's for: See actual packets on the network wire in real time. The ground truth of what's being sent and received.
You don't need to know the flags. You need to know what the output means.
You run tcpdump during a failing test and see:

```
14:32:00.001  IP sender → receiver  UDP  len 64  seq 1
14:32:00.011  IP sender → receiver  UDP  len 64  seq 2
14:32:00.021  IP sender → receiver  UDP  len 64  seq 3
14:32:00.031  IP sender → receiver  UDP  len 64  seq 4
14:32:00.041  IP sender → receiver  UDP  len 64  seq 5
14:32:00.190  IP sender → receiver  UDP  len 64  seq 6
14:32:00.200  IP sender → receiver  UDP  len 64  seq 7
14:32:00.210  IP sender → receiver  UDP  len 64  seq 8
```

** "I'd reach for [tcpdump] when [I suspect of data loss] because [it shows me the packets that are on the network in real time, so I know what is being sent and received]."


### strace

> What it's for: See every single system call a process makes. When a process is stuck and you don't know why — strace tells you exactly what it's waiting for.
You attach strace to a hanging orion_tests process and see:

```
read(7, 0x7fff5a2b1c40, 1024)     = 1024
read(7, 0x7fff5a2b1c40, 1024)     = 1024
read(7, 0x7fff5a2b1c40, 1024)     = 1024
write(1, "[ORION] frame processed\n", 24) = 24
read(7, 0x7fff5a2b1c40, 1024)     = 1024
read(7, 0x7fff5a2b1c40, 1024)     <-- hangs here, no response for 30 seconds
```


>"When I see a hang on fd 7, I'd run ls -la /proc/PID/fd/7 on Linux to see exactly what file or socket that descriptor points to. That tells me precisely what the process is waiting on." 
That command — /proc/PID/fd/ — is one of the most powerful debugging tools in Linux. Every open file descriptor of every running process is visible there.



** "I'd reach for [strace] when [a process is hanging and I don't know why], because [it shows me the exact system call it's blocked on]."


### vmstat + journalctl quick fire

```
r  b   us  sy  id  wa
1  0   15   3  82   0   ← normal
1  0   14   2  84   0   ← normal
4  3   45  12  10  33   ← wa=33, what does this mean?
6  4   42  11   8  39   ← getting worse
1  0   16   3  81   0   ← back to normal

>wa stands for wait — percentage of time CPU is idle waiting for disk I/O. High wa means your disk is the bottleneck, not your CPU.
"If I see high wa during a test run I immediately suspect disk — maybe we're writing logs too frequently, or a database query is doing a full table scan, or the disk is failing."


journalctl — the system diary:
```
journalctl -u orion_tests --since "10 minutes ago"
```

>Shows everything the system logged about orion_tests in the last 10 minutes — start time, any crashes, exit codes, kernel messages.

The most useful flags:

```
journalctl -u SERVICE    # logs for one service
journalctl -p err        # only errors
journalctl --since "5 min ago"  # time window
journalctl -f            # follow in real time (like tail -f)
```
> "When something crashed and I don't know why, journalctl is the first place to look. It captures what happened right before the crash — which top and strace can't because they're live tools."

** "I'd reach for [vmstat] when [I want to rule out disk or CPU] because [it shows me the waiting we are having, so if it is big, I can assume the prolem is not CPU but disk]."
-> "High wa means disk bottleneck. High us means CPU bottleneck. If both are low but the system feels slow, I'd look at network or application logic next."


** "I'd reach for [journalctl] when [I have no clue where to start] because [it is the system diary, it will have everything that happened up until the crash, so then I can start there]."

```
# Show logs for the last 10 minutes
journalctl --since "10 minutes ago"

# Or if it's a specific service:
journalctl -u orion_tests --since "10 minutes ago"

# Filter only errors:
journalctl -p err --since "10 minutes ago"
```


---

### verify environment variable:
> First command to run:
$ printenv | grep ORION