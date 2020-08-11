#include <vector>
#include <cstdlib>

bool test_OS_malloc(int  cnt,  std::size_t each_size)
{
  bool re = true;
  std::vector<void *> vec_allocs;
  vec_allocs.reserve(cnt);
  for (int i = 0; i < cnt; ++i) {
    void *ptr = malloc(each_size);
    if (ptr)
    {
      vec_allocs.emplace_back(ptr);
    }
  }
  for (auto &&it : vec_allocs) {
    free(it);
  }
  return re;
}  // test_OS_malloc
