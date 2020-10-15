# ninja2wctr

Calculates Wall Clock Time Responsibility (WCTR) for each output from `.ninja_log`.

The WCTR metric is not just the duration of a task, it also takes parallelism into account.
E.g. if a task is fully processed in parallel, the WCTR will be lower than when it blocks parallel execution.

See [Faster C++ builds, simplified: a new metric for time](https://devblogs.microsoft.com/cppblog/faster-cpp-builds-simplified-a-new-metric-for-time/) which explains the metric in detail.

## Sample Output

```
      WCTR  wallclock parallel output
     2.171      5.621      2.6 CMakeFiles/nb.dir/src/test/app/doctest.cpp.o
     1.550      5.000      3.2 CMakeFiles/nb.dir/src/test/app/nanobench.cpp.o
     0.142      1.955     13.8 CMakeFiles/nb.dir/src/test/example_random_number_generators.cpp.o
     0.136      0.136      1.0 nb
     0.112      0.739      6.6 CMakeFiles/nb.dir/src/test/unit_templates.cpp.o
     0.086      1.198     14.0 CMakeFiles/nb.dir/src/test/example_random_uniform01.cpp.o
     0.082      1.047     12.8 CMakeFiles/nb.dir/src/test/tutorial_mustache.cpp.o
     0.079      1.090     13.9 CMakeFiles/nb.dir/src/test/tutorial_fluctuating_v1.cpp.o
     0.078      1.096     14.0 CMakeFiles/nb.dir/src/test/example_pyperf.cpp.o
     0.078      1.097     14.0 CMakeFiles/nb.dir/src/test/tutorial_complexity_sort.cpp.o
     0.076      1.065     14.0 CMakeFiles/nb.dir/src/test/example_random2.cpp.o
     0.073      1.013     14.0 CMakeFiles/nb.dir/src/test/example_containers.cpp.o
     0.070      0.729     10.4 CMakeFiles/nb.dir/src/test/unit_romutrio.cpp.o
     0.065      0.542      8.3 CMakeFiles/nb.dir/src/test/unit_to_s.cpp.o
```

* Output `doctest.cpp.o` takes the most WCTR, also because not much is going on in parallel. Thankfully that file doesn't need to be rebuilt often.
* `example_random_number_generators.cpp.o` takes 1.955 seconds of wall clock time, but since on average 13.8 tasks  ran in parallel the WCTR is much lower.
* The result of the build was the binary `nb` which was the final linker step, so nothing was running in parallel to it.
