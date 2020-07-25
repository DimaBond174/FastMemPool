#include "fast_mem_pool.h"

/**
 * @brief test_allocator1
 * @return
 *  Тестируем стандартное использование аллокатора для конструирования объектов
 */

bool test_singletone()
{
  /*
   * Так как не указываем второй параметр шаблона (конкретизация шаблона FastMemPool),
   * то шаблон должен раскрыться на использование FastMemPool<>::instance()
 */
  FastMemPoolAllocator<std::string> myAllocator;
  // allocate space for three strings
  std::string* str = myAllocator.allocate(3);
  // construct these 3 strings
  myAllocator.construct(str, "Mother ");
  myAllocator.construct(str + 1, " washed ");
  myAllocator.construct(str + 2, "the frame");

  //std::cout << str[0] << str[1] << str[2];

  // destroy these 3 strings
  myAllocator.destroy(str);
  myAllocator.destroy(str + 1);
  myAllocator.destroy(str + 2);
  myAllocator.deallocate(str, 3);
  return  true;
}

bool test_Template_method()
{
  /*
   * Инжектируем метод аллокации в Compile time (будет создан SingleTone такого типа)
 */
  FastMemPoolAllocator<std::string, FastMemPool<111, 11> > myAllocator;
  // allocate space for three strings
  std::string* str = myAllocator.allocate(3);
  // construct these 3 strings
  myAllocator.construct(str, "Mother ");
  myAllocator.construct(str + 1, " washed ");
  myAllocator.construct(str + 2, "the frame");

  //std::cout << str[0] << str[1] << str[2];

  // destroy these 3 strings
  myAllocator.destroy(str);
  myAllocator.destroy(str + 1);
  myAllocator.destroy(str + 2);
  myAllocator.deallocate(str, 3);
  return  true;
}

bool test_Strategy()
{
  /*
   * Инжектируем Стратегию аллокации в Runtime (будем использовать конкретный экземпляр такого типа)
 */
  using MyAllocatorType = FastMemPool<333, 33>;
  MyAllocatorType  fastMemPool;  // instance of
  FastMemPoolAllocator<std::string, MyAllocatorType > myAllocator(&fastMemPool); // inject instance
  // allocate space for three strings
  std::string* str = myAllocator.allocate(3);
  // construct these 3 strings
  myAllocator.construct(str, "Mother ");
  myAllocator.construct(str + 1, " washed ");
  myAllocator.construct(str + 2, "the frame");

  //std::cout << str[0] << str[1] << str[2];

  // destroy these 3 strings
  myAllocator.destroy(str);
  myAllocator.destroy(str + 1);
  myAllocator.destroy(str + 2);
  myAllocator.deallocate(str, 3);
  return  true;
}

bool  test_allocator1()
{  
  test_singletone();
  test_Template_method();
  test_Strategy();
  return true;
}
