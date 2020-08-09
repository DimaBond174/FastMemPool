#include "fast_mem_pool.h"

/**
 * @brief test_allocator1
 * @return
 *  Testing the standard use of the allocator for constructing objects
 */


bool test_singletone()
{
  /*
   * Since we do not specify the second parameter of the template (instantiating the FastMemPool template),
   * the template should be expanded to use FastMemPool <> :: instance ()
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
   * Inject the allocation method template at Compile time (a SingleTone of this type will be created)
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
   * We inject the Allocation Strategy instance while Runtime (we will use a specific instance of this type)
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
