#include "../../testing/testing.hpp"

#include "../stipple_pen_resource.hpp"

using namespace dp;

namespace
{
  void TestPacker(StipplePenPacker & packer, uint32_t width, m2::RectU const & expect)
  {
    m2::RectU rect = packer.PackResource(width);
    TEST_EQUAL(rect, expect, ());
  }
}

UNIT_TEST(SimpleStipplePackTest)
{
  StipplePenPacker packer(m2::PointU(1024, 8));
  TestPacker(packer, 30, m2::RectU(1, 1, 31, 2));
  TestPacker(packer, 254, m2::RectU(1, 3, 255, 4));
  TestPacker(packer, 1, m2::RectU(1, 5, 2, 6));
  TestPacker(packer, 250, m2::RectU(256, 1, 506, 2));
  TestPacker(packer, 249, m2::RectU(256, 3, 505, 4));
}

UNIT_TEST(SimpleStippleTest)
{
  StipplePenKey key;
  TEST_EQUAL(key.Tag, StipplePenTag, ());
  key.m_pattern.push_back(12);
  key.m_pattern.push_back(12);
  key.m_pattern.push_back(8);
  key.m_pattern.push_back(9);

  StipplePenResource res(key);
  TEST_EQUAL(res.GetSize(), 246, ());
  TEST_EQUAL(res.GetBufferSize(), 246, ());

  uint8_t buffer[250];
  memset(buffer, 127, ARRAY_SIZE(buffer) * sizeof(uint8_t));
  res.Rasterize(buffer + 2);

  typedef pair<size_t, size_t> TRange;
  typedef pair<TRange, uint8_t> TCheckNode;

  TCheckNode nodes[38] =
  {
    make_pair(make_pair(0, 2), 127),
    make_pair(make_pair(2, 14), 255),
    make_pair(make_pair(14, 26), 0),
    make_pair(make_pair(26, 34), 255),
    make_pair(make_pair(34, 43), 0),
    make_pair(make_pair(43, 55), 255),
    make_pair(make_pair(55, 67), 0),
    make_pair(make_pair(67, 75), 255),
    make_pair(make_pair(75, 84), 0),
    make_pair(make_pair(84, 96), 255),
    make_pair(make_pair(96, 108), 0),
    make_pair(make_pair(108, 116), 255),
    make_pair(make_pair(116, 125), 0),
    make_pair(make_pair(125, 137), 255),
    make_pair(make_pair(137, 149), 0),
    make_pair(make_pair(149, 157), 255),
    make_pair(make_pair(157, 166), 0),
    make_pair(make_pair(166, 178), 255),
    make_pair(make_pair(178, 190), 0),
    make_pair(make_pair(190, 198), 255),
    make_pair(make_pair(198, 207), 0),
    make_pair(make_pair(207, 219), 255),
    make_pair(make_pair(219, 231), 0),
    make_pair(make_pair(231, 239), 255),
    make_pair(make_pair(239, 248), 0),
    make_pair(make_pair(248, 250), 127),
  };

  for (size_t i = 0; i < ARRAY_SIZE(nodes); ++i)
  {
    TCheckNode const & node = nodes[i];
    for (size_t r = node.first.first; r < node.first.second; ++r)
      TEST_EQUAL(buffer[r], node.second, (r));
  }
}
