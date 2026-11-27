# Scale x1
## Mean coverage
 - SGM HH4
   - acc = 85.8799%
   - trials = 500
   - time = 46m 14.5s
   - params = {'p1': 16, 'p2': 60, 'uniqness_ratio': 0, 'filter_radius': 2, 'filter_niter': 1}
 - SGM HH
   - acc = 85.2642%
   - trials = 500
   - time = 60m 5.4s
   - params = {'p1': 16, 'p2': 51, 'uniqness_ratio': 2, 'filter_radius': 1, 'filter_niter': 4}
 - BM
   - acc = 94.9575%
   - trials = 250
   - time = 20m 36.4s
   - params = {'block_size': 19, 'filter_radius': 7, 'filter_niter': 2}
   - notes:
     - Most block sizes have similar results, can be reduced if slow
 - BP
   - acc = 86.7795%
   - trials = 250
   - time = 58m 15.7s
   - params = {'iters': 3, 'levels': 5, 'nr_plane': 7, 'filter_radius': 2, 'filter_niter': 1}
## STDs of diffs
 - SGM HH4
   - std = 35.4676px
   - trials = 500
   - time = 53m 40.9s
   - params = {'p1': 0, 'p2': 0, 'uniqness_ratio': 25, 'filter_radius': 7, 'filter_niter': 7}
   - notes:
    - ultra degenerated
 - SGM HH
   - std = 45.4061px
   - trials = 500
   - time = 67m 15.3s
   - params = {'p1': 14, 'p2': 196, 'uniqness_ratio': 25, 'filter_radius': 7, 'filter_niter': 7}
   - notes:
    - ultra degenerated
 - BM
   - std = 117.8499px
   - trials = 500
   - time = 21m 26.4s
   - params = {'block_size': 13, 'filter_radius': 1, 'filter_niter': 1}
   - notes:
    - close to "mean coverage" version

# Jetson benchmarks
## Scale x1
```
SGM_HH4+filter  avg= 61.02ms std= 20.22ms
SGM_HH4         avg= 31.48ms std= 11.09ms
SGM_HH+filter   avg= 97.52ms std= 43.98ms
SGM_HH          avg= 60.87ms std= 23.96ms
BM+filter       avg= 22.48ms std=  2.62ms
BM              avg= 16.87ms std=  0.25ms
BP+filter       avg=342.44ms std= 10.43ms
BP              avg=343.91ms std=  6.10ms
```
## Scale x0.5
```
SGM_HH4+filter  avg= 15.67ms std=  2.32ms
SGM_HH4         avg=  8.19ms std=  1.11ms
SGM_HH+filter   avg= 25.05ms std=  2.13ms
SGM_HH          avg= 16.04ms std=  3.64ms
BM+filter       avg=  5.62ms std=  0.14ms
BM              avg=  4.31ms std=  0.08ms
BP+filter       avg= 72.04ms std=  2.58ms
BP              avg= 72.58ms std=  0.92ms
```
## Scale x0.25
```
SGM_HH4+filter  avg=  4.53ms std=  1.17ms
SGM_HH4         avg=  2.24ms std=  0.30ms
SGM_HH+filter   avg=  6.74ms std=  0.68ms
SGM_HH          avg=  4.23ms std=  0.52ms
BM+filter       avg=  1.64ms std=  0.11ms
BM              avg=  1.36ms std=  0.02ms
BP+filter       avg= 17.82ms std=  0.89ms
BP              avg= 18.55ms std=  0.74ms
```