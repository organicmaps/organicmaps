#include "drape/drape_tests/dummy_texture.hpp"
#include "drape/drape_tests/glmock_functions.hpp"
#include "drape/drape_tests/img.hpp"

#include "platform/platform.hpp"
#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "drape/drape_routine.hpp"
#include "drape/font_texture.hpp"
#include "drape/glyph_manager.hpp"

#include <functional>

#include <QtCore/QPoint>
#include <QtGui/QPainter>

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::AnyNumber;
using ::testing::Invoke;
using namespace dp;
using namespace std::placeholders;

namespace
{
class UploadedRender
{
public:
  UploadedRender(QPoint const & pen) : m_pen(pen) {}

  void glMemoryToQImage(int x, int y, int w, int h, glConst f, glConst t, void const * memory)
  {
    TEST(f == gl_const::GLAlpha || f == gl_const::GLAlpha8 || f == gl_const::GLRed, ());
    TEST(t == gl_const::GLUnsignedByteType, ());

    uint8_t const * image = reinterpret_cast<uint8_t const *>(memory);

    QPoint p(m_pen);
    p.rx() += x;
    m_images.push_back(qMakePair(p, CreateImage(w, h, image)));
    m_pen.ry() += h;
  }

  void Render(QPaintDevice * device)
  {
    QPainter p(device);
    for (auto d : m_images)
      p.drawImage(d.first, d.second);
  }

private:
  QPoint m_pen;
  QVector<QPair<QPoint, QImage>> m_images;
};

class DummyGlyphIndex : public GlyphIndex
{
  typedef GlyphIndex TBase;

public:
  DummyGlyphIndex(m2::PointU size, ref_ptr<GlyphManager> mng) : TBase(size, mng) {}
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphKey const & key)
  {
    bool dummy = false;
    return TBase::MapResource(key, dummy);
  }
};
}  // namespace

UNIT_TEST(UploadingGlyphs)
{
// This unit test creates window so can't be run in GUI-less Linux machine.
#ifndef OMIM_OS_LINUX
  DrapeRoutine::Init();
  EXPECTGL(glHasExtension(_)).Times(AnyNumber());
  EXPECTGL(glBindTexture(_)).Times(AnyNumber());
  EXPECTGL(glDeleteTexture(_)).Times(AnyNumber());
  EXPECTGL(glTexParameter(_, _)).Times(AnyNumber());
  EXPECTGL(glTexImage2D(_, _, _, _, _)).Times(AnyNumber());
  EXPECTGL(glGenTexture()).Times(AnyNumber());

  UploadedRender r(QPoint(10, 10));
  dp::GlyphManager::Params args;
  args.m_uniBlocks = "unicode_blocks.txt";
  args.m_whitelist = "fonts_whitelist.txt";
  args.m_blacklist = "fonts_blacklist.txt";
  GetPlatform().GetFontNames(args.m_fonts);

  GlyphManager mng(args);
  DummyGlyphIndex index(m2::PointU(128, 128), make_ref(&mng));
  size_t count = 1;  // invalid symbol glyph has mapped internally.
  count += (index.MapResource(GlyphKey(0x58, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x59, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x61, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  while (index.GetPendingNodesCount() < count)
    ;

  Texture::Params p;
  p.m_allocator = GetDefaultAllocator();
  p.m_format = dp::ALPHA;
  p.m_width = p.m_height = 128;

  DummyTexture tex;
  tex.Create(p);
  EXPECTGL(glTexSubImage2D(_, _, _, _, _, _, _))
      .WillRepeatedly(Invoke(&r, &UploadedRender::glMemoryToQImage));
  index.UploadResources(make_ref(&tex));

  count = 0;
  count += (index.MapResource(GlyphKey(0x68, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x30, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x62, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x65, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x400, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  count += (index.MapResource(GlyphKey(0x401, GlyphManager::kDynamicGlyphSize)) != nullptr) ? 1 : 0;
  while (index.GetPendingNodesCount() < count)
    ;

  EXPECTGL(glTexSubImage2D(_, _, _, _, _, _, _))
      .WillRepeatedly(Invoke(&r, &UploadedRender::glMemoryToQImage));
  index.UploadResources(make_ref(&tex));

  RunTestLoop("UploadingGlyphs", std::bind(&UploadedRender::Render, &r, _1));
  DrapeRoutine::Shutdown();
#endif
}
