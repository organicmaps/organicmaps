#include "testing/testing.hpp"

#include "drape/drape_tests/dummy_texture.hpp"
#include "drape/drape_tests/memory_comparer.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"

#include "drape/gl_constants.hpp"
#include "drape/texture.hpp"
#include "drape/texture_of_colors.hpp"

#include "drape/drape_tests/gl_mock_functions.hpp"

#include <gmock/gmock.h>

using testing::_;
using testing::AnyOf;
using testing::IgnoreResult;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using namespace dp;

namespace
{

void TestRects(m2::RectF const & a, m2::RectF const & b)
{
  TEST_ALMOST_EQUAL_ULPS(a.minX(), b.minX(), ());
  TEST_ALMOST_EQUAL_ULPS(a.maxX(), b.maxX(), ());
  TEST_ALMOST_EQUAL_ULPS(a.minY(), b.minY(), ());
  TEST_ALMOST_EQUAL_ULPS(a.maxY(), b.maxY(), ());
}

void InitOpenGLTextures(int const w, int const h)
{
  InSequence seq;
  EXPECTGL(glHasExtension(_)).WillRepeatedly(Return(true));
  EXPECTGL(glGenTexture()).WillOnce(Return(1));
  EXPECTGL(glBindTexture(1)).WillOnce(Return());
  EXPECTGL(glTexImage2D(w, h, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, NULL));
  EXPECTGL(glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear));
  EXPECTGL(glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear));
  EXPECTGL(glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge));
  EXPECTGL(glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge));
  EXPECTGL(glBindTexture(0)).WillOnce(Return());
}

class DummyColorPallete : public ColorPalette
{
  typedef ColorPalette TBase;

public:
  explicit DummyColorPallete(m2::PointU const & size) : TBase(size) {}

  ref_ptr<Texture::ResourceInfo> MapResource(ColorKey const & key)
  {
    bool dummy = false;
    return TBase::MapResource(key, dummy);
  }
};

}  // namespace

UNIT_TEST(ColorPalleteMappingTests)
{
  DummyColorPallete cp(m2::PointU(32, 16));

  ref_ptr<Texture::ResourceInfo> info1 = cp.MapResource(dp::ColorKey(dp::Color(0, 0, 0, 0)));
  ref_ptr<Texture::ResourceInfo> info2 = cp.MapResource(dp::ColorKey(dp::Color(1, 1, 1, 1)));
  ref_ptr<Texture::ResourceInfo> info3 = cp.MapResource(dp::ColorKey(dp::Color(0, 0, 0, 0)));

  TEST_NOT_EQUAL(info1, info2, ());
  TEST_EQUAL(info1, info3, ());

  TestRects(info1->GetTexRect(), m2::RectF(1.0f / 32.0f, 1.0f / 16, 1.0f / 32.0f, 1.0f / 16));
  TestRects(info2->GetTexRect(), m2::RectF(3.0f / 32.0f, 1.0f / 16, 3.0f / 32.0f, 1.0f / 16));
  TestRects(info3->GetTexRect(), m2::RectF(1.0f / 32.0f, 1.0f / 16, 1.0f / 32.0f, 1.0f / 16));

  for (int i = 2; i < 100; ++i)
    cp.MapResource(dp::ColorKey(dp::Color(i, i, i, i)));

  TestRects(cp.MapResource(dp::ColorKey(dp::Color(54, 54, 54, 54)))->GetTexRect(),
            m2::RectF(13.0f / 32.0f, 7.0f / 16.0f, 13.0f / 32.0f, 7.0f / 16.0f));
}

UNIT_TEST(ColorPalleteUploadingSingleRow)
{
  int const width = 32;
  int const height = 16;
  InitOpenGLTextures(width, height);

  TestingGraphicsContext context;
  Texture::Params p;
  p.m_allocator = GetDefaultAllocator(make_ref(&context));
  p.m_format = dp::TextureFormat::RGBA8;
  p.m_width = width;
  p.m_height = height;

  DummyTexture texture;
  texture.Create(make_ref(&context), p);
  DummyColorPallete cp(m2::PointU(width, height));
  cp.UploadResources(make_ref(&context), make_ref(&texture));

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0, 0xFF, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0, 0, 0xFF, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0, 0, 0, 0xFF)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xBB, 0xCC, 0xDD)));

    uint8_t memoryEtalon[] = {
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0x00, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)
        0x00, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)
        0x00, 0x00, 0xFF, 0x00,  // 3 pixel (1st row)
        0x00, 0x00, 0xFF, 0x00,  // 3 pixel (1st row)
        0x00, 0x00, 0x00, 0xFF,  // 4 pixel (1st row)
        0x00, 0x00, 0x00, 0xFF,  // 4 pixel (1st row)
        0xAA, 0xBB, 0xCC, 0xDD,  // 5 pixel (1st row)
        0xAA, 0xBB, 0xCC, 0xDD,  // 5 pixel (1st row)

        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0x00, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)
        0x00, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)
        0x00, 0x00, 0xFF, 0x00,  // 3 pixel (2nd row)
        0x00, 0x00, 0xFF, 0x00,  // 3 pixel (2nd row)
        0x00, 0x00, 0x00, 0xFF,  // 4 pixel (2nd row)
        0x00, 0x00, 0x00, 0xFF,  // 4 pixel (2nd row)
        0xAA, 0xBB, 0xCC, 0xDD,  // 5 pixel (2nd row)
        0xAA, 0xBB, 0xCC, 0xDD   // 5 pixel (2nd row)
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    EXPECTGL(glTexSubImage2D(0, 0, 10, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xAA, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xFF, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0xFF, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0, 0xFF)));
    cp.MapResource(dp::ColorKey(dp::Color(0x00, 0xBB, 0xCC, 0xDD)));

    uint8_t memoryEtalon[] = {
        0xFF, 0xAA, 0x00, 0x00,  // 1 pixel (1st row)
        0xFF, 0xAA, 0x00, 0x00,  // 1 pixel (1st row)
        0xAA, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)
        0xAA, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)
        0xAA, 0x00, 0xFF, 0x00,  // 3 pixel (1st row)
        0xAA, 0x00, 0xFF, 0x00,  // 3 pixel (1st row)
        0xAA, 0x00, 0x00, 0xFF,  // 4 pixel (1st row)
        0xAA, 0x00, 0x00, 0xFF,  // 4 pixel (1st row)
        0x00, 0xBB, 0xCC, 0xDD,  // 5 pixel (1st row)
        0x00, 0xBB, 0xCC, 0xDD,  // 5 pixel (1st row)

        0xFF, 0xAA, 0x00, 0x00,  // 1 pixel (2nd row)
        0xFF, 0xAA, 0x00, 0x00,  // 1 pixel (2nd row)
        0xAA, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)
        0xAA, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)
        0xAA, 0x00, 0xFF, 0x00,  // 3 pixel (2nd row)
        0xAA, 0x00, 0xFF, 0x00,  // 3 pixel (2nd row)
        0xAA, 0x00, 0x00, 0xFF,  // 4 pixel (2nd row)
        0xAA, 0x00, 0x00, 0xFF,  // 4 pixel (2nd row)
        0x00, 0xBB, 0xCC, 0xDD,  // 5 pixel (2nd row)
        0x00, 0xBB, 0xCC, 0xDD   // 5 pixel (2nd row)
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    EXPECTGL(glTexSubImage2D(10, 0, 10, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  EXPECTGL(glDeleteTexture(1));
}

UNIT_TEST(ColorPalleteUploadingPartialyRow)
{
  int const width = 8;
  int const height = 8;
  InitOpenGLTextures(width, height);

  TestingGraphicsContext context;
  Texture::Params p;
  p.m_allocator = GetDefaultAllocator(make_ref(&context));
  p.m_format = dp::TextureFormat::RGBA8;
  p.m_width = width;
  p.m_height = height;

  DummyTexture texture;
  texture.Create(make_ref(&context), p);

  DummyColorPallete cp(m2::PointU(width, height));

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0, 0)));

    uint8_t memoryEtalon[] = {
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel

        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel
        0xFF, 0xFF, 0x00, 0x00   // 2 pixel
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    EXPECTGL(glTexSubImage2D(0, 0, 4, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0xAA, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0xAA, 0xAA)));

    uint8_t memoryEtalon1[] = {
        0xAA, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0xAA, 0x00, 0x00,  // 2 pixel
        0xAA, 0xAA, 0x00, 0x00,  // 2 pixel

        0xAA, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0xAA, 0x00, 0x00,  // 2 pixel
        0xAA, 0xAA, 0x00, 0x00   // 2 pixel
    };

    uint8_t memoryEtalon2[] = {
        0xAA, 0xAA, 0xAA, 0x00,  // 1 pixel
        0xAA, 0xAA, 0xAA, 0x00,  // 1 pixel
        0xAA, 0xAA, 0xAA, 0xAA,  // 2 pixel
        0xAA, 0xAA, 0xAA, 0xAA,  // 2 pixel

        0xAA, 0xAA, 0xAA, 0x00,  // 1 pixel
        0xAA, 0xAA, 0xAA, 0x00,  // 1 pixel
        0xAA, 0xAA, 0xAA, 0xAA,  // 2 pixel
        0xAA, 0xAA, 0xAA, 0xAA   // 2 pixel
    };

    MemoryComparer cmp1(memoryEtalon1, ARRAY_SIZE(memoryEtalon1));
    EXPECTGL(glTexSubImage2D(4, 0, 4, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp1, &MemoryComparer::cmpSubImage));

    MemoryComparer cmp2(memoryEtalon2, ARRAY_SIZE(memoryEtalon2));
    EXPECTGL(glTexSubImage2D(0, 2, 4, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp2, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  EXPECTGL(glDeleteTexture(1));
}

UNIT_TEST(ColorPalleteUploadingMultiplyRow)
{
  int const width = 4;
  int const height = 8;
  InitOpenGLTextures(width, height);

  TestingGraphicsContext context;
  Texture::Params p;
  p.m_allocator = GetDefaultAllocator(make_ref(&context));
  p.m_format = dp::TextureFormat::RGBA8;
  p.m_width = width;
  p.m_height = height;

  DummyTexture texture;
  texture.Create(make_ref(&context), p);

  DummyColorPallete cp(m2::PointU(width, height));
  cp.SetIsDebug(true);

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0, 0)));

    uint8_t memoryEtalon[] = {
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel

        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    EXPECTGL(glTexSubImage2D(0, 0, 4, 2, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xCC, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0xFF, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0xFF, 0xFF)));

    uint8_t memoryEtalon1[] = {
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel

        0xCC, 0x00, 0x00, 0x00,  // 1 pixel
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel

        0xFF, 0xFF, 0xFF, 0x00,  // 1 pixel
        0xFF, 0xFF, 0xFF, 0x00,  // 1 pixel
        0xFF, 0xFF, 0xFF, 0xFF,  // 2 pixel
        0xFF, 0xFF, 0xFF, 0xFF,  // 2 pixel

        0xFF, 0xFF, 0xFF, 0x00,  // 1 pixel
        0xFF, 0xFF, 0xFF, 0x00,  // 1 pixel
        0xFF, 0xFF, 0xFF, 0xFF,  // 2 pixel
        0xFF, 0xFF, 0xFF, 0xFF,  // 2 pixel
    };

    MemoryComparer cmp1(memoryEtalon1, ARRAY_SIZE(memoryEtalon1));
    EXPECTGL(glTexSubImage2D(0, 2, 4, 4, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
        .WillOnce(Invoke(&cmp1, &MemoryComparer::cmpSubImage));

    cp.UploadResources(make_ref(&context), make_ref(&texture));
  }

  EXPECTGL(glDeleteTexture(1));
}
