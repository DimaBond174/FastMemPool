#include "fast_mem_pool.h"
#include <memory>
#include <iostream>

static constexpr int FMALLOC_SIZE  { 1000 };

extern FastMemPool<> *get_singletone1();
extern FastMemPool<> *get_singletone2();

void free1(FastMemPool<>  *fastMemPool,  void  *test_ptr )
{
    FFREE(fastMemPool, test_ptr);
    return;
}

void free2(FastMemPool<>  *fastMemPool,  void  *test_ptr )
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
  std::shared_ptr<FastMemPool<> > test_shared = std::make_shared<FastMemPool<> >();
  void  *test_ptr = FMALLOC(FastMemPool<>::instance(), FMALLOC_SIZE);

  try {
    // Пробуем использовать синглтон из 3-х translation unit-ов,
    // справился ли линковщик:
      free1(get_singletone1(), test_ptr);
      re = false;
      free2(get_singletone2(), test_ptr);
  } catch (std::range_error e) {
      std::cerr << "catched try №1: " << e.what() << std::endl;
      re = !re;
  } catch (...)
  {
    std::cerr << "catched: test_exception1::try №1" << std::endl;
  }

  if (re)
  {
    re = false;
    // Дополнительно попробуем разные экземпляры FastMemPool:
    void  *test_ptr = FMALLOC(test_shared, FMALLOC_SIZE);
    try {
        free1(get_singletone1(), test_ptr);
    } catch (std::range_error e) {
        std::cerr << "catched try №2: " << e.what() << std::endl;
        re = !re;
    } catch (...)
    {
      std::cerr << "catched: test_exception1::try №2" << std::endl;
    }

    FFREE(test_shared, test_ptr);
  }

  return  re;
}
