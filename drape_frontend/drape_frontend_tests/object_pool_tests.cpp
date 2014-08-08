#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../../drape/object_pool.hpp"
#include "../../drape_frontend/read_mwm_task.hpp"

using namespace df;

class vec2
{
public:
  class vec2Factory
  {
  public:
    vec2 * GetNew() const
    {
      return new vec2();
    }
  };

public:
  float x, y;
  void Reset()
  {
    x = y = 0.0f;
  }
};

UNIT_TEST(ObjectPoolFilling)
{
  vec2::vec2Factory factory;
  ObjectPool<vec2, vec2::vec2Factory> pool(1, factory);
  vec2 *pt1, *pt2, *pt3;
  pt1 = pool.Get();
  pt1->x = 45;
  pool.Return(pt1);
  ASSERT_EQUAL(pt1->x, 0, ());
  pt2 = pool.Get();
  ASSERT_EQUAL(pt1, pt2, ());
  pt3 = pool.Get();

  pool.Return(pt2);
  pool.Return(pt3);
}

