#include "fast_mem_pool.h"

using  TVideoPool = FastMemPool<100000, 10, 1000, false, false>;
static std::atomic_bool  keep_detect { true };

// Some kind of video analytics detector
void people_detector(TVideoPool  &video_pool)
{
  return;
}

// Video camera
void video_camera(TVideoPool  &video_pool)
{
  return;
}

/**
 * @brief test_allocator1
 * @return
 *  Тестируем стандартное использование аллокатора для конструирования объектов
 */
bool  test_memcontrol1()
{
  TVideoPool  video_pool;
  keep_detect.store(true, std::memory_order_release);
доделай с SpecStack - камера кладёт в него если может, обработчики забирают
  return true;
}
