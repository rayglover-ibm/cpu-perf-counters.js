# FAQ

## Is this a benchmarking library?

This library exposes hardware performance counters via Node.js; benchmarking libraries could use this to support a wider range of performance analysis, and create more reliable (repeatable) benchmarks for Node.js applications.

## `begin()` raises `Error: Failed to create event (Permission denied)`

This error is typically caused by the default permissions for unprivileged users on newer Linux systems. These permissions are controlled by the  [`perf_event_paranoid`](https://www.kernel.org/doc/Documentation/sysctl/kernel.txt) sysctl file and can be set like so:

```
sudo sysctl -w kernel.perf_event_paranoid=1
```

## Can I use this in AWS/Azure/Virtualised environments?

Hardware counters are generally unavailable in virtualized environments. However the relavent features are [enabled](https://aws.amazon.com/blogs/aws/new-amazon-ec2-bare-metal-instances-with-direct-access-to-hardware/) on some 'bare metal' AWS instances.

Generally speaking to verify that performance monitoring is enabled check the `arch_perfmon` CPU flag is present:

```
grep -c "arch_perfmon" /proc/cpuinfo
```

A result > 0 should indicate that performance monitoring is enabled.

## Why does this exist?

System noise caused by OS jitter and other confounding factors such as CPU frequency scaling and temperature can effect benchmark reproducibility. [1](https://arxiv.org/pdf/1608.04295.pdf)

For this reason a count of CPU cycles is, in general, a more reliable statistic than a timer as a means of measuring and comparing the amount of 'work' a CPU undertook to execute some software.

## How do I achieve reproducible and statistically meaningful results?

Many statistical methods have been developed to try to overcome problems of benchmarking, but these are beyond the scope of this package.

A simple way to reduce the effect of several common sources of system noise, however, is to use the Linux `cset` tool. This tool can confine a program to a single pysical CPU core.

For example:

```bash
# create a shield to run the analysis on a physical # CPU (2 logical processors)
sudo cset shield --cpu=2-3 --kthread=on

# run the analysis
cset shield --exec node -- \
  ./node_modules/.bin/teenytest 'tests/*.test.js'

# reset the shield
cset shield --reset
```