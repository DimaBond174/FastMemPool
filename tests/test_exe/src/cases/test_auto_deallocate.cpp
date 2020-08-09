#include "fast_mem_pool.h"

/**
 * @brief test_auto_deallocate
 * @return
 *  Testing automatic memory return for OS malloc
 */

bool test_auto_deallocate()
{
  FastMemPool<100, 10, 10>  memPool;
  void *ptr = nullptr;
  for (int i = 0; i < 100; ++i) {
    void *ptr = memPool.fmalloc(101);
  }
  if (ptr)
  {
    memPool.ffree(ptr);
  }
  // check mem leak of the process
  return true;
}
