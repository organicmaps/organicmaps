#include "testing/testing.hpp"

#include "drape/drape_tests/memory_comparer.hpp"
#include "drape/drape_tests/dummy_texture.hpp"

#include "drape/glconstants.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/texture.hpp"


#include "drape/drape_tests/glmock_functions.hpp"

#include <gmock/gmock.h>

using testing::_;
using testing::Return;
using testing::InSequence;
using testing::Invoke;
using testing::IgnoreResult;
using testing::AnyOf;
using namespace dp;

namespace
{
  void TestPacker(StipplePenPacker & packer, uint32_t width, m2::RectU const & expect)
  {
    m2::RectU rect = packer.PackResource(width);
    TEST_EQUAL(rect, expect, ());
  }

  bool IsRectsEqual(m2::RectF const & r1, m2::RectF const & r2)
  {
    return my::AlmostEqualULPs(r1.minX(), r2.minX()) &&
           my::AlmostEqualULPs(r1.minY(), r2.minY()) &&
           my::AlmostEqualULPs(r1.maxX(), r2.maxX()) &&
           my::AlmostEqualULPs(r1.maxY(), r2.maxY());
  }

  class DummyStipplePenIndex : public StipplePenIndex
  {
    typedef StipplePenIndex TBase;
  public:
    DummyStipplePenIndex(m2::PointU const & size)
      : TBase(size)
    {
    }

    ref_ptr<Texture::ResourceInfo> MapResource(StipplePenKey const & key)
    {
      bool dummy = false;
      return TBase::MapResource(key, dummy);
    }
  };
}

UNIT_TEST(SimpleStipplePackTest)
{
  StipplePenPacker packer(m2::PointU(512, 8));
  TestPacker(packer, 30, m2::RectU(0, 0, 30, 1));
  TestPacker(packer, 254, m2::RectU(0, 1, 254, 2));
  TestPacker(packer, 1, m2::RectU(0, 2, 1, 3));
  TestPacker(packer, 250, m2::RectU(0, 3, 250, 4));
  TestPacker(packer, 249, m2::RectU(0, 4, 249, 5));

  m2::RectF mapped = packer.MapTextureCoords(m2::RectU(0, 0, 256, 1));
  TEST(IsRectsEqual(mapped, m2::RectF(0.5f / 512.0f, 0.5f / 8.0f,
                                      255.5f / 512.0f, 0.5f / 8.0f)), ());
}

UNIT_TEST(SimplePatternKey)
{
  {
    StipplePenKey info;
    info.m_pattern.push_back(2);
    info.m_pattern.push_back(21);

    TEST_EQUAL(StipplePenHandle(info), StipplePenHandle(0x204A000000000000), ());
  }

  {
    StipplePenKey info;
    info.m_pattern.push_back(1);
    info.m_pattern.push_back(1);
    TEST_EQUAL(StipplePenHandle(info), StipplePenHandle(0x2000000000000000), ());
  }

  {
    StipplePenKey info;
    info.m_pattern.push_back(12);
    info.m_pattern.push_back(12);
    info.m_pattern.push_back(8);
    info.m_pattern.push_back(9);
    info.m_pattern.push_back(128);
    info.m_pattern.push_back(128);
    info.m_pattern.push_back(40);
    info.m_pattern.push_back(40);
    TEST_EQUAL(StipplePenHandle(info), StipplePenHandle(0xE2C58711FFFA74E0), ());
  }
}
