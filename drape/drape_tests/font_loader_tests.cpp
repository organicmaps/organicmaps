#include "../../testing/testing.hpp"
#include "../font_loader.hpp"

#include "glmock_functions.hpp"
#include "../../platform/platform.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::AnyNumber;

void TestCoords(m2::RectF &texRect, float x1, float y1, float x2, float y2)
{
  TEST_ALMOST_EQUAL(texRect.minX(), x1, ("TESTING_TEXTURE_COORDS_FAILURE"))
  TEST_ALMOST_EQUAL(texRect.minY(), y1, ("TESTING_TEXTURE_COORDS_FAILURE"))
  TEST_ALMOST_EQUAL(texRect.maxX(), x2, ("TESTING_TEXTURE_COORDS_FAILURE"))
  TEST_ALMOST_EQUAL(texRect.maxY(), y2, ("TESTING_TEXTURE_COORDS_FAILURE"))
}

void PrepareOpenGL(int size)
{
  EXPECTGL(glGetInteger(gl_const::GLMaxTextureSize)).WillOnce(Return(size));
  EXPECTGL(glBindTexture(_)).Times(AnyNumber());
  EXPECTGL(glDeleteTexture(_)).Times(AnyNumber());
  EXPECTGL(glTexParameter(_, _)).Times(AnyNumber());
  EXPECTGL(glTexImage2D(_, _, _, _, _)).Times(AnyNumber());
  EXPECTGL(glGenTexture()).Times(AnyNumber());
}

UNIT_TEST(SimpleFontLoading_1024)
{
  FontLoader converter;

  PrepareOpenGL(1024);

  vector<TextureFont> tf = converter.Load("test1");
  TEST_EQUAL(converter.GetBlockByUnicode(1), 5, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(2), 10, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(3), 15, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(4), 7, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(5), 13, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(6), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(7), 12, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(8), 14, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(9), 7, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(10), 9, ("TESTING_BLOCK_RESIZING_FAILURE"));

  FontChar symbol;
  TEST(tf[5].GetSymbolByUnicode(1, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 0, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 0, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST(tf[9].GetSymbolByUnicode(10, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 910, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 886, ("TESTING_SYMBOL_COORDS_FAILURE"));

  m2::RectF texRect;
  m2::PointU pixelSize;
  TEST(tf[5].FindResource(TextureFont::FontKey(1), texRect, pixelSize),
                          ("UNICODE_SYMBOL_NOT_FOUND"));
  TestCoords(texRect, 0.0f, 0.0f, 8.0f/1024.0f, 8.0f/1024.0f);
}

UNIT_TEST(SimpleFontLoading_2048)
{
  FontLoader converter;

  PrepareOpenGL(2048);

  vector<TextureFont> tf = converter.Load("test1");
  TEST_EQUAL(converter.GetBlockByUnicode(1), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(2), 3, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(3), 3, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(4), 1, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(5), 2, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(6), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(7), 2, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(8), 3, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(9), 1, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(10), 2, ("TESTING_BLOCK_RESIZING_FAILURE"));

  FontChar symbol;
  TEST(tf[0].GetSymbolByUnicode(1, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 1024, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 1024, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST(tf[2].GetSymbolByUnicode(10, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 1934, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 886, ("TESTING_SYMBOL_COORDS_FAILURE"));

  m2::RectF texRect;
  m2::PointU pixelSize;
  TEST(tf[2].FindResource(TextureFont::FontKey(7), texRect, pixelSize),
                          ("UNICODE_SYMBOL_NOT_FOUND"));
  TestCoords(texRect, 234.0f/2048.0f, 1024.0f/2048.0f, 242.0f/2048.0f, 1035.0f/2048.0f);
}

UNIT_TEST(SimpleFontLoading_4096)
{
  FontLoader converter;

  PrepareOpenGL(4096);

  vector<TextureFont> tf = converter.Load("test1");
  TEST_EQUAL(converter.GetBlockByUnicode(1), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(2), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(3), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(4), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(5), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(6), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(7), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(8), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(9), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));
  TEST_EQUAL(converter.GetBlockByUnicode(10), 0, ("TESTING_BLOCK_RESIZING_FAILURE"));

  FontChar symbol;
  TEST(tf[0].GetSymbolByUnicode(1, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 1024, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 1024, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST(tf[0].GetSymbolByUnicode(10, symbol), ("UNICODE_SYMBOL_NOT_FOUND"));
  TEST_EQUAL(symbol.m_x, 1934, ("TESTING_SYMBOL_COORDS_FAILURE"));
  TEST_EQUAL(symbol.m_y, 2934, ("TESTING_SYMBOL_COORDS_FAILURE"));

  m2::RectF texRect;
  m2::PointU pixelSize;
  TEST(tf[0].FindResource(TextureFont::FontKey(3), texRect, pixelSize),
                          ("UNICODE_SYMBOL_NOT_FOUND"));
  TestCoords(texRect, 3072.0f/4096.0f, 3072.0f/4096.0f, 3080.0f/4096.0f, 3087.0f/4096.0f);
}
