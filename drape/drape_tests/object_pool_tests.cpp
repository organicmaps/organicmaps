#include "testing/testing.hpp"

#include "drape/object_pool.hpp"

class vec2
{
public:
  class vec2Factory
  {
  public:
    vec2 * GetNew() const { return new vec2(); }
  };

public:
  float m_x, m_y;
  static int m_counter;

  vec2() : m_x(0.0f), m_y(0.0f) { m_counter++; }
  ~vec2() { m_counter--; }
};

int vec2::m_counter = 0;

UNIT_TEST(ObjectPoolFilling)
{
  vec2::vec2Factory factory;
  dp::ObjectPool<vec2, vec2::vec2Factory> pool(1, factory);
  vec2 *pt1, *pt2, *pt3;
  pt1 = pool.Get();
  pool.Return(pt1);
  pt2 = pool.Get();
  TEST_EQUAL(pt1, pt2, ());
  pt3 = pool.Get();

  pool.Return(pt2);
  pool.Return(pt3);
}

UNIT_TEST(ObjectPoolClearing_1)
{
  vec2::m_counter = 0;
  vec2::vec2Factory factory;
  auto pool = new dp::ObjectPool<vec2, vec2::vec2Factory>(1, factory);
  vec2 *pt1, *pt2, *pt3;
  pt1 = pool->Get();
  pool->Return(pt1);
  pt2 = pool->Get();
  pt3 = pool->Get();

  pool->Return(pt2);
  pool->Return(pt3);

  delete pool;

  TEST_EQUAL(vec2::m_counter, 0, ());
}

UNIT_TEST(ObjectPoolClearing_2)
{
  vec2::m_counter = 0;
  vec2::vec2Factory factory;
  auto pool = new dp::ObjectPool<vec2, vec2::vec2Factory>(100, factory);
  vec2 *pt1, *pt2, *pt3;
  pt1 = pool->Get();
  pool->Return(pt1);
  pt2 = pool->Get();
  pt3 = pool->Get();

  pool->Return(pt2);
  pool->Return(pt3);

  delete pool;

  TEST_EQUAL(vec2::m_counter, 0, ());
}
