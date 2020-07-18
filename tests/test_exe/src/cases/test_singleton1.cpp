#include  "fast_mem_pool.h"

FastMemPool<> &get_singletone1()
{
  return FastMemPool<>::instance();
}
