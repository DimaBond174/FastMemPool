#include "fast_mem_pool.h"


bool test_fastmempool(int  cnt,  std::size_t each_size)
{
  bool re = true;
  //std::vector<void *> vec_allocs;
  //vec_allocs.reserve(cnt);

  // FastMemPool Constructor will takes time here:
  FastMemPool<16000000, 16, 16, false, false>  fastMemPool;
  for (int i = 0; i < cnt; ++i) {
    void *ptr = fastMemPool.fmalloc(each_size);
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

  // FastMemPool Destructor will takes time here..
  return re;
} // test_fastmempool
