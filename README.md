# Threadpool Benchmarks
Which threadpool is the fastest? A naive centralized lock one? OpenMP? Grand Central Dispatch? Look here!
```
|             ns/task |              task/s |    err% |     total | TaskingOverhead
|--------------------:|--------------------:|--------:|----------:|:----------------
|              144.54 |        6,918,284.48 |    1.5% |     17.79 | `GCD`
|            1,071.72 |          933,077.90 |    2.2% |    128.98 | `Custom`
|            3,399.83 |          294,132.17 |    9.9% |    385.60 | :wavy_dash: `OpenMP` (Unstable with ~11.1 iters. Increase `minEpochIterations` to e.g. 111)
```

Surprisingly, OpenMP seems to be really bad on MacOS, at least in this benchmark. Need further investigation about why. Grand Central Dispatch is fastest by an order of magnitude, suggesting it's good enough to build your parallel apps on it. 

It would also be good to test Intel TBB, but I haven't gotten around to it. 


