#include  "fast_mem_pool.h"

// An diff translation unit

FastMemPool<> *get_singletone2()
{
  return FastMemPool<>::instance();
}
