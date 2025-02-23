## C++ Lock Free Bounded Queue Implementation With Analysis

## ⚙️ Build & Run
```shell
git clone git@github.com:b1tflyyyy/lock-free-bounded-queue.git

git submodule update --init --recursive

mkdir build && cd build

cmake -G $GENERATOR -DCMAKE_C_COMPILER=$C_COMPILER -DCMAKE_CXX_COMPILER=$CXX_COMPILER -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

[build command depends on the $GENERATOR]
```

## 👻 Options
```shell
[unit-tests options]
-DENABLE_LEAK_SANITIZER=ON/OFF
-DENABLE_THREAD_SANITIZER=ON/OFF
-DPRINT_RES_BUF=ON/OFF -> [enables/disables printing results from the buffer]

[profiling options]
-DSHOW_RESULTS=ON/OFF [the same as -DPRINT_RES_BUF but for profiling]
-DUSE_THREAD_YIELD=ON/OFF [enables/disables using std::this_thread::yield() in the loop]
```

## 🚀 Profiling
_Foreword_: 
- I used the Tracy profiler.
- Compilation options: taken from the cmake release configuration.

_Tests were ran with several options_:
- class members aligning with the cache line size [to avoid false sharing] `[enabled/disabled]`
- using `std::this_thread::yield()` in the 'spinlock' `[enabled/disabled]`
- different producer/consumer thread counts

_General information about the test machine_:
- CPU -> `AMD Ryzen 7 4800H 8/16`
- RAM -> `DDR4 3200MHz 16GB Dual Channel`

_Profiling Table_
- 1p_1c -> 1 producer, 1 consumer
- Queue capacity -> 1024 tasks max
- Our testing task creates an array with random numbers `[Complexity O(N)]` and then sorts it using `BubbleSort` `[Complexity O(N^2)]`
- Size of array -> 2048 objects of `std::ptrdiff_t` type
- Task count -> 100'000

|Producer, Consumer|Alignment|Yield|Time      |
|------------------|---------|-----|----------|
|1p_1c             |YES      |YES  |2min 40sec|
|1p_1c             |YES      |NO   |3min 10sec|
|1p_1c             |NO       |NO   |3min 10sec|
|2p_2c             |YES      |YES  |1min 20sec|
|2p_2c             |YES      |NO   |1min 37sec|
|2p_2c             |NO       |NO   |1min 37sec|
|4p_4c             |YES      |YES  |42sec     |
|4p_4c             |YES      |NO   |51sec     |
|4p_4c             |NO       |NO   |51sec     |
|8p_8c             |YES      |YES  |31sec     |
|8p_8c             |YES      |NO   |39sec     |
|8p_8c             |NO       |NO   |36sec     |
|16p_16c           |YES      |YES  |30sec     |
|16p_16c           |YES      |NO   |43sec     |
|16p_16c           |NO       |NO   |43sec     |
