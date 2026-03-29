#include "testing/testing.hpp"

#include "drape/drape_tests/dummy_texture.hpp"
#include "drape/drape_tests/memory_comparer.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"

#include "drape/gl_constants.hpp"
#include "drape/texture.hpp"
#include "drape/texture_of_colors.hpp"

#include "drape/drape_tests/gl_mock_functions.hpp"

#include <gmock/gmock.h>

namespace texture_of_colors_tests
{
using testing::_;
using testing::AnyOf;
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
  EXPECTGL(glHasExtension(_)).WillRepeatedly(Return(true));
  EXPECTGL(glGenTexture()).WillOnce(Return(1));
  EXPECTGL(glBindTexture(1, gl_const::GLTexture2D)).WillOnce(Return());
  EXPECTGL(glTexImage2D(w, h, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, NULL));
  EXPECTGL(glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear, gl_const::GLTexture2D));
  EXPECTGL(glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear, gl_const::GLTexture2D));
  EXPECTGL(glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge, gl_const::GLTexture2D));
  EXPECTGL(glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge, gl_const::GLTexture2D));
  EXPECTGL(glBindTexture(0, gl_const::GLTexture2D)).WillOnce(Return());
}

/// Set up bind/unbind expectations for N upload calls, then add per-upload glTexSubImage2D expectations.
void ExpectUploads(int count)
{
  EXPECTGL(glBindTexture(1, gl_const::GLTexture2D)).Times(count);
  EXPECTGL(glBindTexture(0, gl_const::GLTexture2D)).Times(count);
}

void ExpectUploadData(MemoryComparer & cmp, int x, int y, int w, int h)
{
  EXPECTGL(glTexSubImage2D(x, y, w, h, AnyOf(gl_const::GLRGBA, gl_const::GLRGBA8), gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp, &MemoryComparer::cmpSubImage));
}

class DummyColorPalette : public ColorPalette
{
  using TBase = ColorPalette;

public:
  explicit DummyColorPalette(m2::PointU const & size) : TBase(size) {}

  ref_ptr<Texture::ResourceInfo> MapResource(ColorKey const & key)
  {
    bool dummy = false;
    return TBase::MapResource(key, dummy);
  }
};

class TestColorPaletteFixture
{
  TestingGraphicsContext m_context;
  std::unique_ptr<DummyTexture> m_texture;
  DummyColorPalette m_cp;

public:
  TestColorPaletteFixture(int width, int height) : m_cp(m2::PointU(width, height))
  {
    InitOpenGLTextures(width, height);

    Texture::Params p;
    p.m_allocator = GetDefaultAllocator(make_ref(&m_context));
    p.m_format = dp::TextureFormat::RGBA8;
    p.m_width = width;
    p.m_height = height;

    m_texture = std::make_unique<DummyTexture>();
    m_texture->Create(make_ref(&m_context), p);

    // Clear expectations from InitOpenGLTextures after texture creation.
    testing::Mock::VerifyAndClearExpectations(&emul::GLMockFunctions::Instance());
  }

  DummyColorPalette & GetPalette() { return m_cp; }
  void UploadResources() { m_cp.UploadResources(make_ref(&m_context), make_ref(m_texture.get())); }

  ~TestColorPaletteFixture()
  {
    EXPECTGL(glDeleteTexture(1));
    m_texture.reset();

    bool const ok = testing::Mock::VerifyAndClearExpectations(&emul::GLMockFunctions::Instance());
    TEST(ok, ("GMock expectations not met"));
  }
};

}  // namespace

UNIT_TEST(ColorPaletteMappingTests)
{
  DummyColorPalette cp(m2::PointU(32, 16));

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

UNIT_TEST(ColorPaletteUploadingSingleRow)
{
  TestColorPaletteFixture fixture(32, 16);
  fixture.UploadResources();
  auto & cp = fixture.GetPalette();

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
    ExpectUploads(1);
    ExpectUploadData(cmp, 0, 0, 10, 2);
    fixture.UploadResources();
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
    ExpectUploads(1);
    ExpectUploadData(cmp, 10, 0, 10, 2);
    fixture.UploadResources();
  }
}

UNIT_TEST(ColorPaletteUploadingPartialyRow)
{
  TestColorPaletteFixture fixture(8, 8);
  fixture.UploadResources();
  auto & cp = fixture.GetPalette();

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
    ExpectUploads(1);
    ExpectUploadData(cmp, 0, 0, 4, 2);
    fixture.UploadResources();
  }

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0xAA, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0xAA, 0xAA, 0xAA)));

    // 4 colors: 2 fit in row 0 (x=4..7), 2 go to row 1 (x=0..3). Two separate uploads.
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

    ExpectUploads(2);
    MemoryComparer cmp1(memoryEtalon1, ARRAY_SIZE(memoryEtalon1));
    ExpectUploadData(cmp1, 4, 0, 4, 2);

    MemoryComparer cmp2(memoryEtalon2, ARRAY_SIZE(memoryEtalon2));
    ExpectUploadData(cmp2, 0, 2, 4, 2);

    fixture.UploadResources();
  }
}

UNIT_TEST(ColorPaletteUploadingMultiplyRow)
{
  TestColorPaletteFixture fixture(4, 8);
  fixture.UploadResources();
  auto & cp = fixture.GetPalette();

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xAA, 0, 0, 0)));

    uint8_t memoryEtalon[] = {
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel (1st row)
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel (1st row)

        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0xFF, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel (2nd row)
        0xAA, 0x00, 0x00, 0x00,  // 2 pixel (2nd row)
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    ExpectUploads(1);
    ExpectUploadData(cmp, 0, 0, 4, 2);
    fixture.UploadResources();
  }

  {
    cp.MapResource(dp::ColorKey(dp::Color(0xCC, 0, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0xFF, 0)));
    cp.MapResource(dp::ColorKey(dp::Color(0xFF, 0xFF, 0xFF, 0xFF)));

    // 4 colors span 2 rows: (0xCC, 0xFFFF) at y=2, (0xFFFFFF, 0xFFFFFFFF) at y=4.
    // Single upload covering both rows.
    uint8_t memoryEtalon[] = {
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel (1st row)
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel (1st row)

        0xCC, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0xCC, 0x00, 0x00, 0x00,  // 1 pixel (2nd row)
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)
        0xFF, 0xFF, 0x00, 0x00,  // 2 pixel (2nd row)

        0xFF, 0xFF, 0xFF, 0x00,  // 3 pixel (3rd row)
        0xFF, 0xFF, 0xFF, 0x00,  // 3 pixel (3rd row)
        0xFF, 0xFF, 0xFF, 0xFF,  // 4 pixel (3rd row)
        0xFF, 0xFF, 0xFF, 0xFF,  // 4 pixel (3rd row)

        0xFF, 0xFF, 0xFF, 0x00,  // 3 pixel (4th row)
        0xFF, 0xFF, 0xFF, 0x00,  // 3 pixel (4th row)
        0xFF, 0xFF, 0xFF, 0xFF,  // 4 pixel (4th row)
        0xFF, 0xFF, 0xFF, 0xFF,  // 4 pixel (4th row)
    };

    MemoryComparer cmp(memoryEtalon, ARRAY_SIZE(memoryEtalon));
    ExpectUploads(1);
    ExpectUploadData(cmp, 0, 2, 4, 4);
    fixture.UploadResources();
  }
}
}  // namespace texture_of_colors_tests
