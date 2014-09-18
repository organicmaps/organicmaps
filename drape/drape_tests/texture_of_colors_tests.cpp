#include "../../testing/testing.hpp"

#include "memory_comparer.hpp"

#include "../glconstants.hpp"
#include "../texture_of_colors.hpp"
#include "../texture_set_holder.hpp"
#include "../texture.hpp"

#include "glmock_functions.hpp"

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
class SimpleTexture : public Texture
{
public:
  SimpleTexture() {}

  virtual ResourceInfo const * FindResource(Key const & key) const { return NULL; }
};

void TestRects(m2::RectF const & a, m2::RectF const & b)
{
  TEST_ALMOST_EQUAL(a.minX(), b.minX(), ());
  TEST_ALMOST_EQUAL(a.maxX(), b.maxX(), ());
  TEST_ALMOST_EQUAL(a.minY(), b.minY(), ());
  TEST_ALMOST_EQUAL(a.maxY(), b.maxY(), ());
}

void InitOpenGLTextures(int const w, int const h)
{
  InSequence seq;
  EXPECTGL(glHasExtension(_)).WillRepeatedly(Return(true));
  EXPECTGL(glGenTexture()).WillOnce(Return(1));
  EXPECTGL(glBindTexture(1)).WillOnce(Return());
  EXPECTGL(glTexImage2D(w, h, gl_const::GLRGBA, gl_const::GL8BitOnChannel, NULL));
  EXPECTGL(glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear));
  EXPECTGL(glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear));
  EXPECTGL(glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge));
  EXPECTGL(glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge));
}
}

UNIT_TEST(ColorPalleteMappingTests)
{
  ColorPalette cp(m2::PointU(32, 16));

  dp::ColorKey key(0);
  key.SetColor(0);
  ColorResourceInfo const * info1 = cp.MapResource(key);
  key.SetColor(1);
  ColorResourceInfo const * info2 = cp.MapResource(key);
  key.SetColor(0);
  ColorResourceInfo const * info3 = cp.MapResource(key);

  TEST_NOT_EQUAL(info1, info2, ());
  TEST_EQUAL(info1, info3, ());

  for (int i = 2; i < 100; ++i)
  {
    key.SetColor(i);
    cp.MapResource(key);
  }

  key.SetColor(54);
  ColorResourceInfo const * info4 = cp.MapResource(key);
  ColorResourceInfo const info5(m2::RectF(22.5f / 32.0f, 1.5f / 16.0f, 22.5f / 32.0f, 1.5f / 16.0f));
  TestRects(info4->GetTexRect(), info5.GetTexRect());
}

UNIT_TEST(ColorPalleteUploadingTests1)
{
  int const width = 32;
  int const height = 16;
  InitOpenGLTextures(width, height);

  SimpleTexture texture;
  texture.Create(width, height, dp::RGBA8);
  ColorPalette cp(m2::PointU(width, height));
  cp.UploadResources(MakeStackRefPointer<Texture>(&texture));

  dp::ColorKey key(0);
  key.SetColor(0);
  cp.MapResource(key);
  key.SetColor(1);
  cp.MapResource(key);
  key.SetColor(2);
  cp.MapResource(key);
  key.SetColor(3);
  cp.MapResource(key);
  key.SetColor(4);
  cp.MapResource(key);

  int ar1[5] = {0, 1, 2, 3, 4};
  int ar2[27];
  int ar3[64];
  int ar4[4];

  MemoryComparer cmp1(ar1, sizeof(int) * 5);
  EXPECTGL(glTexSubImage2D(0, 0, 5, 1, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp1, &MemoryComparer::cmpSubImage));

  cp.UploadResources(MakeStackRefPointer<Texture>(&texture));

  for (int i = 5; i < 32; ++i)
  {
    key.SetColor(i);
    cp.MapResource(key);
    ar2[i - 5] = i;
  }

  for (int i = 32; i < 96; ++i)
  {
    key.SetColor(i);
    cp.MapResource(key);
    ar3[i - 32] = i;
  }

  for (int i = 96; i < 100; ++i)
  {
    key.SetColor(i);
    cp.MapResource(key);
    ar4[i - 96] = i;
  }

  InSequence seq1;
  MemoryComparer cmp2(ar2, sizeof(int) * 27);
  EXPECTGL(glTexSubImage2D(5, 0, 27, 1, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp2, &MemoryComparer::cmpSubImage));

  MemoryComparer cmp3(ar3, sizeof(int) * 64);
  EXPECTGL(glTexSubImage2D(0, 1, 32, 2, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp3, &MemoryComparer::cmpSubImage));

  MemoryComparer cmp4(ar4, sizeof(int) * 4);
  EXPECTGL(glTexSubImage2D(0, 3, 4, 1, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp4, &MemoryComparer::cmpSubImage));

  cp.UploadResources(MakeStackRefPointer<Texture>(&texture));

  EXPECTGL(glDeleteTexture(1));
}

UNIT_TEST(ColorPalleteUploadingTests2)
{
  int const width = 32;
  int const height = 16;
  InitOpenGLTextures(width, height);

  SimpleTexture texture;
  dp::ColorKey key(0);
  texture.Create(width, height, dp::RGBA8);
  ColorPalette cp(m2::PointU(width, height));

  int ar1[32];
  int ar2[1];

  for (int i = 0; i < 32; ++i)
  {
    key.SetColor(i);
    cp.MapResource(key);
    ar1[i] = i;
  }

  MemoryComparer cmp1(ar1, sizeof(int) * 32);
  EXPECTGL(glTexSubImage2D(0, 0, 32, 1, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp1, &MemoryComparer::cmpSubImage));
  cp.UploadResources(MakeStackRefPointer<Texture>(&texture));

  key.SetColor(32);
  cp.MapResource(key);
  ar2[0] = 32;

  MemoryComparer cmp2(ar2, sizeof(int) * 1);
  EXPECTGL(glTexSubImage2D(0, 1, 1, 1, gl_const::GLRGBA, gl_const::GL8BitOnChannel, _))
      .WillOnce(Invoke(&cmp2, &MemoryComparer::cmpSubImage));
  cp.UploadResources(MakeStackRefPointer<Texture>(&texture));

  EXPECTGL(glDeleteTexture(1));
}

