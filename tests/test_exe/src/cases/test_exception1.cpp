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
 * Testing double deallocation
 *  Тестируем двойную деаллокацию
 * @return true
 */
bool  test_exception1()
{
  int  re = 1;
  std::shared_ptr<FastMemPool<> > test_shared = std::make_shared<FastMemPool<> >();
  void  *test_ptr = FMALLOC(FastMemPool<>::instance(), FMALLOC_SIZE);

  try {
    // Trying to use a singleton from 3 translation units, did the linker succeed:
    // Пробуем использовать синглтон из 3-х translation unit-ов, справился ли линковщик:
      free1(get_singletone1(), test_ptr);
      free2(get_singletone2(), test_ptr);
    /*
     * The test proves that Singltone is evil: when multiple threads work with one
       Singltone allocator, it can be difficult to achieve a stable double deallocation error:
       another thread manages to allocate and the second deallocation is then also successful ..
       while another thread catches the error already on the first free1.

      Тест доказывает что Singltone - это зло: когда несколько потоков работают с одним
      аллокатором, добиться стабильной ошибки двойной деаллокации бывает сложно:
      другой поток успевает аллоцировать и вторая деаллокация тогда тоже успешна..
      в то время как другой поток поймает ошибку уже на первой деаллокации..
    */
  } catch (std::range_error e) {
      //std::cerr << "catched try 1: " << e.what() << std::endl;
      re = 2;
  } catch (...)
  {
   // std::cerr << "catched: test_exception1::try 1" << std::endl;
  }

  if (1 == re || 2 == re)
  {
    /*
     * Additionally, let's try different instances of FastMemPool:
       we allocate in one, and we free1 in another

       Дополнительно попробуем разные экземпляры FastMemPool:
      аллоцируем в одном, а деаллоцируем в другом
    */
    void  *test_ptr = FMALLOC(test_shared, FMALLOC_SIZE);
    try {
        free1(get_singletone1(), test_ptr);
    } catch (std::range_error e) {
        //std::cerr << "catched try 2: " << e.what() << std::endl;
        re = 3;
    } catch (...)
    {
      //std::cerr << "catched: test_exception1::try 2" << std::endl;
    }

    FFREE(test_shared, test_ptr);
  }

  if (3 != re)
  {
    std::cerr  <<  "test_exception1::re=="  <<  re  <<  std::endl;
  }
  return  re == 3;
}
