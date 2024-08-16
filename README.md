# Threadpool Benchmarks
Which threadpool is the fastest? A naive centralized lock one? OpenMP? Grand Central Dispatch? Look here!
```
|             ns/task |              task/s |    err% |     total | TaskingOverhead
|--------------------:|--------------------:|--------:|----------:|:----------------
|              146.86 |        6,809,180.93 |    0.8% |     17.92 | `GCD`
|            1,106.50 |          903,748.24 |    1.4% |    131.69 | `Custom`
|            2,354.41 |          424,735.59 |    3.5% |    646.36 | `OpenMP`
```

Surprisingly, OpenMP seems to be really bad on MacOS, at least in this benchmark. Need further investigation about why. Grand Central Dispatch is fastest by an order of magnitude, suggesting it's good enough to build your parallel apps on it. 

It would also be good to test Intel TBB, but I haven't gotten around to it. 


