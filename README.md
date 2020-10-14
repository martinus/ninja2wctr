# ninja2wctr

Calculates Wall Clock Time Responsibility (WCTR) for each output from `.ninja_log`.

The WCTR metric is not just the duration of a task, it also takes parallelism into account.
E.g. if a task is fully processed in parallel, the WCTR will be lower than when it blocks parallel execution.

See [Faster C++ builds, simplified: a new metric for time](https://devblogs.microsoft.com/cppblog/faster-cpp-builds-simplified-a-new-metric-for-time/) which explains the metric in detail.
