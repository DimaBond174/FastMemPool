#include "fast_mem_pool.h"

/**
 * @brief test_allocator1
 * @return
 *  Тестируем стандартное использование аллокатора для конструирования объектов
 */
bool  test_allocator1()
{
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
