#include  "fast_mem_pool.h"

FastMemPool<> *get_singletone2()
{
  return FastMemPool<>::instance();
}
