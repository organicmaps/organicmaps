#include "../../testing/testing.hpp"
#include "../font_texture.hpp"

#include "glmock_functions.hpp"
#include "../../base/stl_add.hpp"
#include "../../platform/platform.hpp"

#include "../../geometry/rect2d.hpp"
#include "../../geometry/point2d.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::AnyNumber;

namespace
{
  void PrepareOpenGL(int size)
  {
    EXPECTGL(glGetInteger(gl_const::GLMaxTextureSize)).WillOnce(Return(size));
    EXPECTGL(glBindTexture(_)).Times(AnyNumber());
    EXPECTGL(glDeleteTexture(_)).Times(AnyNumber());
    EXPECTGL(glTexParameter(_, _)).Times(AnyNumber());
    EXPECTGL(glTexImage2D(_, _, _, _, _)).Times(AnyNumber());
    EXPECTGL(glGenTexture()).Times(AnyNumber());
  }

  FontTexture::GlyphInfo const * FindSymbol(int uni, vector<MasterPointer<Texture> > const & textures,
                                            int & w, int & h)
  {
    FontTexture::GlyphKey key(uni);
    for (size_t i = 0; i < textures.size(); ++i)
    {
      RefPointer<Texture> texture = textures[i].GetRefPointer();
      Texture::ResourceInfo const * info = texture->FindResource(key);
      if (info != NULL)
      {
        w = texture->GetWidth();
        h = texture->GetHeight();
        return static_cast<FontTexture::GlyphInfo const *>(info);
      }
    }

    ASSERT(false, ());
    return NULL;
  }

  void TestSymbol(FontTexture::GlyphInfo const * srcInfo, FontTexture::GlyphInfo dstInfo, int texW, int texH)
  {
    m2::RectF srcTexRect = srcInfo->GetTexRect();
    m2::RectU srcRect(srcTexRect.minX() * texW, srcTexRect.minY() * texH,
                      srcTexRect.maxX() * texW, srcTexRect.maxY() * texH);

    m2::RectF dstTexRect = dstInfo.GetTexRect();
    m2::RectU dstRect(dstTexRect.minX() * texW, dstTexRect.minY() * texH,
                      dstTexRect.maxX() * texW, dstTexRect.maxY() * texH);

    TEST_EQUAL(dstRect, srcRect, ());

    float srcXoff, srcYoff, srcAdvance;
    srcInfo->GetMetrics(srcXoff, srcYoff, srcAdvance);
    float dstXoff, dstYoff, dstAdvance;
    dstInfo.GetMetrics(dstXoff, dstYoff, dstAdvance);

    TEST_ALMOST_EQUAL(srcXoff, dstXoff, ());
    TEST_ALMOST_EQUAL(srcYoff, dstYoff, ());
    TEST_ALMOST_EQUAL(srcAdvance, dstAdvance, ());
  }

  class Tester
  {
  public:
    Tester(int textureSize)
    {
      PrepareOpenGL(textureSize);
    }

    void AddInfo(int id, FontTexture::GlyphInfo const & info)
    {
      m_infos.push_back(make_pair(id, info));
    }

    void TestThis(string const & resPath)
    {
      vector<MasterPointer<Texture> > textures;
      vector<TransferPointer<Texture> > tempTextures;
      LoadFont(resPath, tempTextures);
      textures.reserve(tempTextures.size());
      for (size_t i = 0; i < tempTextures.size(); ++i)
        textures.push_back(MasterPointer<Texture>(tempTextures[i]));

      for (size_t i = 0; i < m_infos.size(); ++i)
      {
        int id = m_infos[i].first;

        int w, h;
        FontTexture::GlyphInfo const * info = FindSymbol(id, textures, w, h);
        TestSymbol(info, m_infos[i].second, w, h);
      }

      DeleteRange(textures, MasterPointerDeleter());
    }

  private:
    vector<pair<int, FontTexture::GlyphInfo> > m_infos;
  };
}
typedef FontTexture::GlyphInfo glyph_t;

UNIT_TEST(CutTextureTest_1024)
{
  Tester t(1024);
  int w = 1024;
  int h = 1024;
  t.AddInfo(1, glyph_t(m2::RectF(0.0   / w, 0.0   / h,
                                 20.0  / w, 20.0  / h), 0.0, 0.0, 0.1));
  t.AddInfo(2, glyph_t(m2::RectF(20.0  / w, 20.0  / h,
                                 45.0  / w, 45.0  / h), 2.0, 4.3, 0.2));
  t.AddInfo(3, glyph_t(m2::RectF(512.0 / w, 256.0 / h,
                                 768.0 / w, 512.0 / h), 0.1, 0.2, 1.2));
  t.AddInfo(4, glyph_t(m2::RectF(768.0 / w, 512.0 / h,
                                 868.0 / w, 612.0 / h), 0.8, 1.0, 1.3));

  t.TestThis("font_test");
}

UNIT_TEST(CutTextureTest_512)
{
  Tester t(512);
  int w = 512;
  int h = 512;
  t.AddInfo(1, glyph_t(m2::RectF(0.0   / w, 0.0   / h,
                                 20.0  / w, 20.0  / h), 0.0, 0.0, 0.1));
  t.AddInfo(2, glyph_t(m2::RectF(20.0  / w, 20.0  / h,
                                 45.0  / w, 45.0  / h), 2.0, 4.3, 0.2));
  t.AddInfo(3, glyph_t(m2::RectF(0.0   / w, 256.0 / h,
                                 256.0 / w, 512.0 / h), 0.1, 0.2, 1.2));
  t.AddInfo(4, glyph_t(m2::RectF(256.0 / w, 0.0   / h,
                                 356.0 / w, 100.0 / h), 0.8, 1.0, 1.3));

  t.TestThis("font_test");
}

UNIT_TEST(RectangleCut_1024)
{
  Tester t(1024);
  int w1 = 512;
  int h1 = 512;
  int w2 = 256;
  int h2 = 256;
  t.AddInfo(1, glyph_t(m2::RectF(0.0   / w1, 0.0   / h1,
                                 20.0  / w1, 20.0  / h1), 0.0, 0.0, 0.1));
  t.AddInfo(2, glyph_t(m2::RectF(0.0   / w1, 412.0 / h1,
                                 100.0 / w1, 512.0 / h1), 2.0, 4.3, 0.2));
  t.AddInfo(3, glyph_t(m2::RectF(0.0   / w2, 0.0   / h2,
                                 50.0  / w2, 50.0  / h2), 0.1, 0.2, 1.2));

  t.TestThis("font_test2");
}


UNIT_TEST(RectangleCut_256)
{
  Tester t(256);
  int w = 256;
  int h = 256;
  t.AddInfo(1, glyph_t(m2::RectF(0.0   / w, 0.0   / h,
                                 20.0  / w, 20.0  / h), 0.0, 0.0, 0.1));
  t.AddInfo(2, glyph_t(m2::RectF(0.0   / w, 156.0 / h,
                                 100.0 / w, 256.0 / h), 2.0, 4.3, 0.2));
  t.AddInfo(3, glyph_t(m2::RectF(0.0   / w, 0.0   / h,
                                 50.0  / w, 50.0  / h), 0.1, 0.2, 1.2));

  t.TestThis("font_test2");
}
