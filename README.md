# FastMemPool
Fast thread-safe C++ allocator with memory access control functions.
All implementation in one header file:
[fast_mem_pool.h](https://github.com/DimaBond174/FastMemPool/blob/master/include/fast_mem_pool.h)

# Base usage
Instead of malloc() and free():

```c++

#include "fast_mem_pool.h"

int main(int argc, char** argv)
{
  FastMemPool<>  fastMemPool;
  void *ptr = fastMemPool.fmalloc(12345);
  fastMemPool.ffree(ptr);
}

```
See [test_base_usage.cpp](https://github.com/DimaBond174/FastMemPool/blob/master/include/fast_mem_pool.h) full example.

# Checking access rights and buffer overflows
See full example here: [test_random_access1.cpp](https://github.com/DimaBond174/FastMemPool/blob/master/tests/test_exe/src/cases/test_random_access1.cpp)
The example uses the following methods:

```c++
// allocate RAM with aka malloc
//  + in Debug mode store info about line number
//   of code where this operation was called:
void  * ptr  =  FMALLOC(fastMemPool, size);

//Checking whether this is my allocation and whether
//I will go beyond the allocation limits if I perform an operation
// on this piece of memory:
if (FCHECK_ACCESS(fastMemPool, elem.array, &elem.array[elem.array_size - 1], sizeof (int))) {
  elem.array[elem.array_size - 1] = rand();
}

// free with checking if it has been freed twice:
FFREE(fastMemPool,  it.array);

```
