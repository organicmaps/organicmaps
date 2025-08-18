#include "testing/testing.hpp"

#include "geometry/packer.hpp"

static int i = 1;

void rectOverflowFn1()
{
  i = i + 5;
}

void rectOverflowFn2()
{
  i *= 2;
}

UNIT_TEST(PackerTest_SimplePack)
{
  m2::Packer p(20, 20);

  m2::Packer::handle_t h0 = p.pack(10, 10);

  p.addOverflowFn(rectOverflowFn2, 10);
  p.addOverflowFn(rectOverflowFn1, 0);

  TEST_EQUAL(p.isPacked(h0), true, ());
  m2::RectU r0 = p.find(h0).second;

  TEST_EQUAL(r0, m2::RectU(0, 0, 10, 10), ());

  m2::Packer::handle_t h1 = p.pack(20, 10);

  TEST_EQUAL(p.isPacked(h1), true, ());
  m2::RectU r1 = p.find(h1).second;
  TEST_EQUAL(r1, m2::RectU(0, 10, 20, 20), ());

  m2::Packer::handle_t h2 = p.pack(5, 5);

  // Possibly we should restore this checks

  //  TEST_EQUAL(p.isPacked(h0), false, ());
  //  TEST_EQUAL(p.isPacked(h1), false, ());

  TEST_EQUAL(p.isPacked(h2), true, ());

  TEST_EQUAL(i, 7, ("Handlers priorities doesn't work"));
  TEST_NOT_EQUAL(i, 12, ("Handlers priorities doesn't work"));

  m2::RectU r2 = p.find(h2).second;

  TEST_EQUAL(r2, m2::RectU(0, 0, 5, 5), ());
}

UNIT_TEST(PackerTest_HasRoom_Sequence)
{
  m2::Packer p(20, 20);

  m2::PointU pts[] = {m2::PointU(10, 10), m2::PointU(11, 3), m2::PointU(5, 5), m2::PointU(5, 5)};

  TEST(p.hasRoom(pts, sizeof(pts) / sizeof(m2::PointU)), ());

  m2::PointU pts1[] = {m2::PointU(10, 10), m2::PointU(11, 3), m2::PointU(5, 5), m2::PointU(5, 5), m2::PointU(16, 5)};

  TEST(!p.hasRoom(pts1, sizeof(pts1) / sizeof(m2::PointU)), ());

  m2::PointU pts2[] = {m2::PointU(10, 10), m2::PointU(11, 3), m2::PointU(5, 5), m2::PointU(5, 5), m2::PointU(10, 6)};

  TEST(!p.hasRoom(pts2, sizeof(pts2) / sizeof(m2::PointU)), ());

  m2::PointU pts3[] = {m2::PointU(10, 10), m2::PointU(11, 3), m2::PointU(5, 5), m2::PointU(5, 5), m2::PointU(15, 5)};

  TEST(p.hasRoom(pts3, sizeof(pts3) / sizeof(m2::PointU)), ());

  m2::PointU pts4[] = {m2::PointU(10, 10), m2::PointU(11, 3), m2::PointU(5, 5), m2::PointU(5, 5), m2::PointU(16, 5)};

  TEST(!p.hasRoom(pts4, sizeof(pts4) / sizeof(m2::PointU)), ());
}
