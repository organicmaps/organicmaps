#include "testing/testing.hpp"

#include "map/screen_coverage.hpp"

#include "map/coverage_generator.hpp"
#include "map/tile_renderer.hpp"
#include "yg/rendercontext.hpp"
#include "std/bind.hpp"
#include "platform/platform.hpp"
#include "yg/internal/opengl.hpp"

class RenderContextMock : public yg::gl::RenderContext
{
public:

  void makeCurrent()
  {}

  shared_ptr<yg::gl::RenderContext> createShared()
  {
    return shared_ptr<yg::gl::RenderContext>(new RenderContextMock());
  }

  void endThreadDrawing()
  {}
};

class TileRendererMock : public TileRenderer
{
public:
  TileRendererMock(string const & skinName,
                   unsigned scaleEtalonSize,
                   unsigned maxTilesCount,
                   unsigned tasksCount,
                   yg::Color const & bgColor,
                   RenderPolicy::TRenderFn const & renderFn)
    : TileRenderer(skinName, scaleEtalonSize, maxTilesCount, tasksCount, bgColor, renderFn)
  {}

  void DrawTile(core::CommandsQueue::Environment const & env,
                Tiler::RectInfo const & rectInfo,
                int sequenceID)
  {
    shared_ptr<yg::gl::BaseTexture> tileTarget = m_resourceManager->renderTargets().Front(true);

    shared_ptr<yg::InfoLayer> layer(new yg::InfoLayer());

    ScreenBase frameScreen;

    unsigned tileWidth = m_resourceManager->tileTextureWidth();
    unsigned tileHeight = m_resourceManager->tileTextureHeight();

    m2::RectI renderRect(1, 1, tileWidth - 1, tileHeight - 1);

    frameScreen.OnSize(renderRect);
    frameScreen.SetFromRect(rectInfo.m_rect);

    LOG(LINFO, ("drawTile : ", rectInfo.m_y, rectInfo.m_x, rectInfo.m_tileScale, rectInfo.m_drawScale, ", id=", rectInfo.toUInt64Cell()));

    if (sequenceID < m_sequenceID)
      return;

    if (HasTile(rectInfo))
      return;

    threads::Sleep(500);

    AddTile(rectInfo, Tile(tileTarget, layer, frameScreen, rectInfo, 0));
  }
};

class WindowHandleMock : public WindowHandle
{
public:
  void invalidateImpl()
  {}
};

void foo()
{}

void ScreenCoverageTestImpl(int executorsNum)
{
  yg::gl::g_doFakeOpenGLCalls = true;

  shared_ptr<WindowHandle> windowHandle(new WindowHandleMock());
  shared_ptr<yg::gl::RenderContext> primaryContext(new RenderContextMock());
  shared_ptr<yg::ResourceManager> resourceManager = make_shared_ptr(new yg::ResourceManager(
      30000 * sizeof(yg::gl::Vertex),
      50000 * sizeof(unsigned short),
      20,
      3000 * sizeof(yg::gl::Vertex),
      5000 * sizeof(unsigned short),
      100,
      10 * sizeof(yg::gl::AuxVertex),
      10 * sizeof(unsigned short),
      30,
      512, 256, 10,
      512, 256, 5,
      512, 512, 20,
      "unicode_blocks.txt",
      "fonts_whitelist.txt",
      "fonts_blacklist.txt",
      2 * 1024 * 1024,
      GetPlatform().CpuCores() + 1,
      yg::Rt8Bpp,
      false));

  TileRendererMock tileRenderer("basic.skn", 512 + 256, 10, executorsNum, yg::Color(), bind(&foo));

  CoverageGenerator gen(
        GetPlatform().TileSize(),
        GetPlatform().ScaleEtalonSize(),
        &tileRenderer,
        windowHandle
        );

  gen.Initialize();
  tileRenderer.Initialize(primaryContext, resourceManager, 1);

  ScreenBase screen0(m2::RectI(0, 0, 347, 653), m2::RectD(14.672034256593292412, 56.763875246806442476, 16.337299222955756761, 59.897644765638396791));
  ScreenBase screen1(m2::RectI(0, 0, 347, 653), m2::RectD(13.539462118087413955, 56.759076212321247112, 15.204727084449878305, 59.892845731153201427));
  ScreenBase screen2(m2::RectI(0, 0, 347, 653), m2::RectD(16.154935912518372021, 56.749478143350856385, 17.82020087888083637, 59.8832476621828107));
  ScreenBase screen3(m2::RectI(0, 0, 347, 653), m2::RectD(17.335498395876193456, 56.725482970924879567, 19.000763362238657805, 59.859252489756833882));

  gen.AddCoverScreenTask(screen0);

  /// waiting for CoverScreen processed
  gen.WaitForEmptyAndFinished();
  /// waiting for the tiles to be rendered...
  tileRenderer.WaitForEmptyAndFinished();
  /// ...and merged
  gen.WaitForEmptyAndFinished();

  gen.AddCoverScreenTask(screen1);

  gen.WaitForEmptyAndFinished();
  tileRenderer.WaitForEmptyAndFinished();
  gen.WaitForEmptyAndFinished();

  gen.AddCoverScreenTask(screen0);

  gen.WaitForEmptyAndFinished();
  tileRenderer.WaitForEmptyAndFinished();
  gen.WaitForEmptyAndFinished();

  /// checking that erased tiles are completely unlocked

  TileCache * tileCache = &tileRenderer.GetTileCache();

  vector<Tiler::RectInfo> erasedRects;

  erasedRects.push_back(Tiler::RectInfo(8, 8, 9, 40));
  erasedRects.push_back(Tiler::RectInfo(8, 8, 9, 41));
  erasedRects.push_back(Tiler::RectInfo(8, 8, 9, 42));

  for (unsigned i = 0; i < erasedRects.size(); ++i)
    CHECK(tileCache->lockCount(erasedRects[i]) == 0, ());

  /// checking, that tiles in coverage are present and locked

  vector<Tiler::RectInfo> coveredRects;

  coveredRects.push_back(Tiler::RectInfo(8, 8, 10, 40));
  coveredRects.push_back(Tiler::RectInfo(8, 8, 10, 41));
  coveredRects.push_back(Tiler::RectInfo(8, 8, 10, 42));

  for (unsigned i = 0; i < coveredRects.size(); ++i)
    CHECK(tileCache->lockCount(coveredRects[i]) > 0, (coveredRects[i].m_x, coveredRects[i].m_y, coveredRects[i].m_tileScale, coveredRects[i].m_drawScale, coveredRects[i].toUInt64Cell()));

  gen.AddCoverScreenTask(screen2);
  gen.WaitForEmptyAndFinished();
  tileRenderer.WaitForEmptyAndFinished();
  gen.WaitForEmptyAndFinished();

  for (unsigned i = 0; i < coveredRects.size(); ++i)
    CHECK(tileCache->lockCount(coveredRects[i]) == 0, (coveredRects[i].m_x, coveredRects[i].m_y, coveredRects[i].m_tileScale, coveredRects[i].m_drawScale, coveredRects[i].toUInt64Cell()));

  coveredRects.clear();

  coveredRects.push_back(Tiler::RectInfo(8, 8, 11, 40));
  coveredRects.push_back(Tiler::RectInfo(8, 8, 11, 41));
  coveredRects.push_back(Tiler::RectInfo(8, 8, 11, 42));

  for (unsigned i = 0; i < coveredRects.size(); ++i)
    CHECK(tileCache->lockCount(coveredRects[i]) == 0, (coveredRects[i].m_x, coveredRects[i].m_y, coveredRects[i].m_tileScale, coveredRects[i].m_drawScale, coveredRects[i].toUInt64Cell()));

  gen.AddCoverScreenTask(screen3);
  gen.WaitForEmptyAndFinished();
  tileRenderer.WaitForEmptyAndFinished();
  gen.WaitForEmptyAndFinished();

  gen.Cancel();
}

UNIT_TEST(ScreenCoverageTest)
{
  ScreenCoverageTestImpl(2);
//  ScreenCoverageTestImpl(2);
}

