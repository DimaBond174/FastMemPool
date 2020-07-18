#include "fast_mem_pool.h"

extern FastMemPool<> &get_singletone1();
extern FastMemPool<> &get_singletone2();

void free1(FastMemPool<>  &fastMemPool,  void  *test_ptr )
{
    FFREE(fastMemPool, test_ptr);
    return;
}

void free2(FastMemPool<>  &fastMemPool,  void  *test_ptr )
{
  FFREE(fastMemPool, test_ptr);
  return;
}

/**
 * @brief test_exception1
 * @return
 *  Тестируем двойную деаллокацию
 */
bool  test_exception1()
{
  bool re = true;
    void  *test_ptr = FMALLOC(FastMemPool<>::instance(), 100000);
    try {
      free1(get_singletone1(), test_ptr);
      re = false;
      free2(get_singletone2(), test_ptr);
    } catch (std::range_error e) {
      //std::cerr << e.what() << std::endl;
      re = !re;
    }
    return  re;
}
