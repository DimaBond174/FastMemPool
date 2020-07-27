#include "fast_mem_pool.h"

/**
 * @brief test_base_usage
 * @return
 *  Тестируем использование вместо malloc() и free().
 *  Test  instead of malloc() and for free() usage.
 */
bool  test_base_usage()
{
    FastMemPool<>  fastMemPool;
    int  *my_array[1000];
    // allocate RAM with aka malloc:
    for (int i  =  0;  i < 500; ++i) {
      my_array[i]  =  static_cast<int *>(fastMemPool.fmalloc(rand() % 256 + 32));
    }
    // allocate RAM with aka malloc
    //  + in Debug mode store info about line number of code where this operation was called
    for (int i  =  500;  i < 1000; ++i) {
      my_array[i]  =  static_cast<int *>(FMALLOC(&fastMemPool, (rand() % 256 + 32)));
    }
    // check if we can access with memory, if it our allocation:
    for (int i  =  0;  i < 1000; ++i) {
      int id = rand() % 4;
      if (FCHECK_ACCESS(&fastMemPool, my_array[i], &my_array[i][id], sizeof(int)))
      {
        my_array[i][id] = rand() % 100;
      }
    }
    // free memory
    for (int i  =  0;  i < 500; ++i) {
      fastMemPool.ffree(my_array[i]);
    }
    // free with checking if it has been freed twice:
    for (int i  =  500;  i < 1000; ++i) {
      FFREE(&fastMemPool, my_array[i]);
    }
    return  true;
}
