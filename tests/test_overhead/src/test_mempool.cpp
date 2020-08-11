#include "mem_pool.h"

bool test_mempool(int  cnt,  std::size_t each_size)
{
  bool re = true;
  //std::vector<void *> vec_allocs;
  //vec_allocs.reserve(cnt);

  // MemPool Constructor will takes time here:
  MemPool<16000000, 16, 16, false, false>  memPool;
  for (int i = 0; i < cnt; ++i) {
    void *ptr = memPool.fmalloc(each_size);
//    if (ptr)
//    {
//      all allocations in FastMemPool, no need vec_allocs for free
//      vec_allocs.emplace_back(ptr);
//    }
  }

//  for (auto &&it : vec_allocs) {
//      all allocations in FastMemPool, no need vec_allocs for free
//    free(it);
//  }

  // MemPool Destructor will takes time here..
  return re;
} // test_mempool
