#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"
#include "../std/set.hpp"
#include "../std/algorithm.hpp"

#include "../indexer/scales.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/opengl/base_texture.hpp"

#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "window_handle.hpp"
#include "coverage_generator.hpp"

ScreenCoverage::ScreenCoverage()
  : m_sequenceID(numeric_limits<int>::max()),
    m_tileRenderer(NULL),
    m_coverageGenerator(NULL),
    m_overlay(new graphics::Overlay()),
    m_isBenchmarking(false),
    m_isEmptyDrawingCoverage(false),
    m_isEmptyModelAtCoverageCenter(true),
    m_leafTilesToRender(0)
{
  m_overlay->setCouldOverlap(false);
}

ScreenCoverage::ScreenCoverage(TileRenderer * tileRenderer,
                               CoverageGenerator * coverageGenerator,
                               shared_ptr<graphics::Screen> const & cacheScreen)
  : m_sequenceID(numeric_limits<int>::max()),
    m_tileRenderer(tileRenderer),
    m_coverageGenerator(coverageGenerator),
    m_overlay(new graphics::Overlay()),
    m_isBenchmarking(false),
    m_isEmptyDrawingCoverage(false),
    m_isEmptyModelAtCoverageCenter(true),
    m_leafTilesToRender(0),
    m_cacheScreen(cacheScreen)
{
  m_overlay->setCouldOverlap(false);
}

void ScreenCoverage::CopyInto(ScreenCoverage & cvg, bool mergeOverlay)
{
  cvg.m_tileRenderer = m_tileRenderer;
  cvg.m_tiler = m_tiler;
  cvg.m_screen = m_screen;
  cvg.m_coverageGenerator = m_coverageGenerator;
  cvg.m_tileRects = m_tileRects;
  cvg.m_newTileRects = m_newTileRects;
  cvg.m_newLeafTileRects = m_newLeafTileRects;
  cvg.m_isEmptyDrawingCoverage = m_isEmptyDrawingCoverage;
  cvg.m_isEmptyModelAtCoverageCenter = m_isEmptyModelAtCoverageCenter;
  cvg.m_leafTilesToRender = m_leafTilesToRender;
  cvg.m_countryIndexAtCoverageCenter = m_countryIndexAtCoverageCenter;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  cvg.m_tiles = m_tiles;

  for (TTileSet::const_iterator it = cvg.m_tiles.begin(); it != cvg.m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->LockTile(ri);
  }

  tileCache->Unlock();
  if (mergeOverlay)
  {
    graphics::Overlay::Lock guard(cvg.m_overlay);
    cvg.m_overlay->merge(*m_overlay);
  }
}

void ScreenCoverage::Clear()
{
  m_tileRects.clear();
  m_newTileRects.clear();
  m_newLeafTileRects.clear();
  {
    graphics::Overlay::Lock guard(m_overlay);
    m_overlay->clear();
  }
  m_isEmptyDrawingCoverage = false;
  m_isEmptyModelAtCoverageCenter = true;
  m_leafTilesToRender = 0;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->UnlockTile(ri);
  }

  tileCache->Unlock();

  m_tiles.clear();
}

void ScreenCoverage::Merge(Tiler::RectInfo const & ri)
{
  ASSERT(m_tileRects.find(ri) != m_tileRects.end(), ());

  m_tileRenderer->CacheActiveTile(ri);
  TileCache & tileCache = m_tileRenderer->GetTileCache();
  tileCache.Lock();

  Tile const * tile = NULL;
  if (tileCache.HasTile(ri))
  {
    tileCache.LockTile(ri);
    tile = &tileCache.GetTile(ri);
  }

  if (tile != NULL)
  {
    m_tiles.insert(tile);
    m_tileRects.erase(ri);
    m_newTileRects.erase(ri);
    m_newLeafTileRects.erase(ri);

    if (m_tiler.isLeaf(ri))
    {
      m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;
      m_leafTilesToRender--;
    }
  }

  tileCache.Unlock();

  if (tile != NULL && m_tiler.isLeaf(ri))
  {
    graphics::Overlay::Lock guard(m_overlay);
    m_overlay->merge(*tile->m_overlay,
                      tile->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());
  }

  //else
  //  LOG(LDEBUG, ("UVRLOG : Tile not found s=", ri.m_tileScale, " x=", ri.m_x, " y=", ri.m_y));
}

void FilterElementsBySharpness(shared_ptr<graphics::OverlayElement> const & e,
                               vector<shared_ptr<graphics::OverlayElement> > & v,
                               bool flag)
{
  if (e->hasSharpGeometry() == flag)
    v.push_back(e);
}

bool ScreenCoverage::Cache(core::CommandsQueue::Environment const & env)
{
  /// caching tiles blitting commands.

  m_primaryDL.reset();
  m_primaryDL.reset(m_cacheScreen->createDisplayList());

  m_sharpTextDL.reset();
  m_sharpTextDL.reset(m_cacheScreen->createDisplayList());

  m_cacheScreen->setEnvironment(&env);

  m_cacheScreen->beginFrame();
  m_cacheScreen->setDisplayList(m_primaryDL.get());

  vector<graphics::BlitInfo> infos;

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tile const * tile = *it;

    size_t tileWidth = tile->m_renderTarget->width();
    size_t tileHeight = tile->m_renderTarget->height();

    graphics::BlitInfo bi;

    bi.m_matrix = tile->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix();
    bi.m_srcRect = m2::RectI(0, 0, tileWidth - 2, tileHeight - 2);
    bi.m_texRect = m2::RectU(1, 1, tileWidth - 1, tileHeight - 1);
    bi.m_srcSurface = tile->m_renderTarget;

    infos.push_back(bi);
  }

  if (!infos.empty())
    m_cacheScreen->blit(&infos[0], infos.size(), true, graphics::minDepth);

  math::Matrix<double, 3, 3> idM = math::Identity<double, 3>();

  // selecting and rendering non-sharp elements.

  {
    graphics::Overlay::Lock guard(m_overlay);
    vector<shared_ptr<graphics::OverlayElement> > nonSharpElements;
    m_overlay->forEach(bind(&FilterElementsBySharpness, _1, ref(nonSharpElements), false));

    for (unsigned i = 0; i < nonSharpElements.size(); ++i)
      nonSharpElements[i]->draw(m_cacheScreen.get(), idM);

    // selecting and rendering sharp elements

    vector<shared_ptr<graphics::OverlayElement> > sharpElements;
    m_overlay->forEach(bind(&FilterElementsBySharpness, _1, ref(sharpElements), true));

    m_cacheScreen->applySharpStates();
    m_cacheScreen->setDisplayList(m_sharpTextDL.get());

    for (unsigned i = 0; i < sharpElements.size(); ++i)
      sharpElements[i]->draw(m_cacheScreen.get(), idM);
  } /// Overlay lock

  m_cacheScreen->setDisplayList(0);
  m_cacheScreen->applyStates();

  m_cacheScreen->endFrame();

  /// completing commands that was immediately executed
  /// while recording of displayList(for example UnlockStorage)

  m_cacheScreen->completeCommands();

  bool isCancelled = m_cacheScreen->isCancelled();

  m_cacheScreen->setEnvironment(0);

  return !isCancelled;
}

void ScreenCoverage::SetScreen(ScreenBase const & screen)
{
  //LOG(LDEBUG, ("UVRLOG : Start ScreenCoverage::SetScreen. m_SequenceID=", GetSequenceID()));
  m_screen = screen;

  m_newTileRects.clear();

  m_tiler.seed(m_screen, m_screen.GlobalRect().GlobalCenter(), m_tileRenderer->TileSize());

  vector<Tiler::RectInfo> allRects;
  vector<Tiler::RectInfo> newRects;
  TTileSet tiles;

  m_tiler.tiles(allRects, GetPlatform().PreCachingDepth());

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  m_isEmptyDrawingCoverage = true;
  m_isEmptyModelAtCoverageCenter = true;
  m_leafTilesToRender = 0;

  for (size_t i = 0; i < allRects.size(); ++i)
  {
    Tiler::RectInfo const & ri = allRects[i];
    m_tileRects.insert(ri);

    if (tileCache->HasTile(ri))
    {
      tileCache->TouchTile(ri);
      Tile const * tile = &tileCache->GetTile(ri);
      ASSERT(tiles.find(tile) == tiles.end(), ());

      if (m_tiler.isLeaf(ri))
        m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;

      tiles.insert(tile);
    }
    else
    {
      newRects.push_back(ri);
      if (m_tiler.isLeaf(ri))
        ++m_leafTilesToRender;
    }
  }

  /// computing difference between current and previous coverage
  /// tiles, that aren't in current coverage are unlocked to allow their deletion from TileCache
  /// tiles, that are new to the current coverage are added into m_tiles and locked in TileCache

  size_t firstTileForAdd = 0;
  buffer_vector<const Tile *, 64> diff_tiles;
  diff_tiles.reserve(m_tiles.size() + tiles.size());
  set_difference(m_tiles.begin(), m_tiles.end(), tiles.begin(), tiles.end(), back_inserter(diff_tiles), TTileSet::key_compare());
  firstTileForAdd = diff_tiles.size();
  set_difference(tiles.begin(), tiles.end(), m_tiles.begin(), m_tiles.end(), back_inserter(diff_tiles), TTileSet::key_compare());

  for (size_t i = 0; i < firstTileForAdd; ++i)
    tileCache->UnlockTile(diff_tiles[i]->m_rectInfo);

  for (size_t i = firstTileForAdd; i < diff_tiles.size(); ++i)
    tileCache->LockTile(diff_tiles[i]->m_rectInfo);

  tileCache->Unlock();

  m_tiles = tiles;

  MergeOverlay();

  vector<Tiler::RectInfo> firstClassTiles;
  vector<Tiler::RectInfo> secondClassTiles;

  unsigned newRectsCount = newRects.size();

  for (unsigned i = 0; i < newRectsCount; ++i)
  {
    Tiler::RectInfo nr = newRects[i];
    //LOG(LDEBUG, ("UVRLOG : NewRect add s=", nr.m_tileScale, " x=", nr.m_x, " y=", nr.m_y, " m_SequenceID=", GetSequenceID()));

    int const step = GetPlatform().PreCachingDepth() - 1;

    if ((nr.m_tileScale == m_tiler.tileScale() - step)
     || (nr.m_tileScale == m_tiler.tileScale() ))
      firstClassTiles.push_back(nr);
    else
      secondClassTiles.push_back(nr);
  }

  /// clearing all old commands
  m_tileRenderer->ClearCommands();
  /// setting new sequenceID
  m_tileRenderer->SetSequenceID(GetSequenceID());
  //LOG(LDEBUG, ("UVRLOG : Cancel commands from set rect. m_SequenceID =", GetSequenceID()));
  m_tileRenderer->CancelCommands();

  // filtering out rects that are fully covered by its descedants

  int curNewTile = 0;

  if (m_isBenchmarking)
    m_coverageGenerator->StartTileDrawingSession(GetSequenceID(), newRectsCount);

  // adding commands for tiles which aren't in cache
  for (size_t i = 0; i < firstClassTiles.size(); ++i, ++curNewTile)
  {
    Tiler::RectInfo const & ri = firstClassTiles[i];

    core::CommandsQueue::Chain chain;

    chain.addCommand(bind(&CoverageGenerator::MergeTile,
                          m_coverageGenerator,
                          ri,
                          GetSequenceID()));

    if (m_isBenchmarking)
    {
      chain.addCommand(bind(&CoverageGenerator::DecrementTileCount,
                            m_coverageGenerator,
                            GetSequenceID()));
    }

    m_tileRenderer->AddCommand(ri, GetSequenceID(),
                               chain);

    if (m_tiler.isLeaf(ri))
      m_newLeafTileRects.insert(ri);

    m_newTileRects.insert(ri);
  }

  for (size_t i = 0; i < secondClassTiles.size(); ++i, ++curNewTile)
  {
    Tiler::RectInfo const & ri = secondClassTiles[i];

    core::CommandsQueue::Chain chain;

    chain.addCommand(bind(&TileRenderer::CacheActiveTile,
                          m_tileRenderer,
                          ri));

    if (m_isBenchmarking)
    {
      chain.addCommand(bind(&CoverageGenerator::DecrementTileCount,
                            m_coverageGenerator,
                            GetSequenceID()));
    }

    m_tileRenderer->AddCommand(ri, GetSequenceID(), chain);
  }
}

ScreenCoverage::~ScreenCoverage()
{
  Clear();
}

void ScreenCoverage::Draw(graphics::Screen * s, ScreenBase const & screen)
{
  math::Matrix<double, 3, 3> m = m_screen.PtoGMatrix() * screen.GtoPMatrix();

  if (m_primaryDL)
    s->drawDisplayList(m_primaryDL.get(), m);

  if (m_sharpTextDL)
    s->drawDisplayList(m_sharpTextDL.get(), m);
}

shared_ptr<graphics::Overlay> const & ScreenCoverage::GetOverlay() const
{
  return m_overlay;
}

int ScreenCoverage::GetDrawScale() const
{
  return m_tiler.tileScale();
}

bool ScreenCoverage::IsEmptyDrawingCoverage() const
{
  return (m_leafTilesToRender <= 0) && m_isEmptyDrawingCoverage;
}

bool ScreenCoverage::IsEmptyModelAtCoverageCenter() const
{
  return m_isEmptyModelAtCoverageCenter;
}

void ScreenCoverage::ResetEmptyModelAtCoverageCenter()
{
  m_isEmptyModelAtCoverageCenter = false;
}

storage::TIndex ScreenCoverage::GetCountryIndexAtCoverageCenter() const
{
  return m_countryIndexAtCoverageCenter;
}

void ScreenCoverage::CheckEmptyModelAtCoverageCenter()
{
  if (!IsPartialCoverage() && IsEmptyDrawingCoverage())
  {
    m2::PointD const centerPt = m_screen.GlobalRect().GetGlobalRect().Center();
    m_countryIndexAtCoverageCenter = m_coverageGenerator->GetCountryIndex(centerPt);
    m_isEmptyModelAtCoverageCenter = m_countryIndexAtCoverageCenter.IsValid();
  }
}

bool ScreenCoverage::IsPartialCoverage() const
{
  return !m_newTileRects.empty();
}

void ScreenCoverage::SetSequenceID(int id)
{
  m_sequenceID = id;
}

int ScreenCoverage::GetSequenceID() const
{
  return m_sequenceID;
}

void ScreenCoverage::RemoveTiles(m2::AnyRectD const & r, int startScale)
{
  vector<Tile const*> toRemove;

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;

    if (r.IsIntersect(m2::AnyRectD(ri.m_rect)) && (ri.m_tileScale >= startScale))
      toRemove.push_back(*it);
  }

  TileCache * tileCache = &m_tileRenderer->GetTileCache();
  tileCache->Lock();

  for (vector<Tile const *>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->UnlockTile(ri);
    m_tiles.erase(*it);
    m_tileRects.erase(ri);
  }
  tileCache->Unlock();

  MergeOverlay();
}

void ScreenCoverage::MergeOverlay()
{
  graphics::Overlay::Lock guard(m_overlay);
  m_overlay->clear();

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    if (m_tiler.isLeaf(ri))
      m_overlay->merge(*(*it)->m_overlay, (*it)->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());
  }
}

void ScreenCoverage::SetBenchmarkingFlag(bool isBenchmarking)
{
  m_isBenchmarking = isBenchmarking;
}
