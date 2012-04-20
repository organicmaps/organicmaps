#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"
#include "../std/set.hpp"
#include "../std/algorithm.hpp"

#include "../indexer/scales.hpp"

#include "../yg/screen.hpp"
#include "../yg/base_texture.hpp"

#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "window_handle.hpp"
#include "coverage_generator.hpp"

ScreenCoverage::ScreenCoverage()
  : m_tiler(),
    m_infoLayer(new yg::InfoLayer()),
    m_isEmptyDrawingCoverage(false),
    m_isEmptyModelAtCoverageCenter(true),
    m_leafTilesToRender(0),
    m_stylesCache(0)
{
  m_infoLayer->setCouldOverlap(false);
}

ScreenCoverage::ScreenCoverage(TileRenderer * tileRenderer,
                               CoverageGenerator * coverageGenerator)
  : m_tileRenderer(tileRenderer),
    m_infoLayer(new yg::InfoLayer()),
    m_isEmptyDrawingCoverage(false),
    m_isEmptyModelAtCoverageCenter(true),
    m_leafTilesToRender(0),
    m_coverageGenerator(coverageGenerator),
    m_stylesCache(0)
{
  m_infoLayer->setCouldOverlap(false);
}

ScreenCoverage * ScreenCoverage::Clone()
{
  ScreenCoverage * res = new ScreenCoverage();

  res->m_tileRenderer = m_tileRenderer;
  res->m_tiler = m_tiler;
  res->m_screen = m_screen;
  res->m_coverageGenerator = m_coverageGenerator;
  res->m_tileRects = m_tileRects;
  res->m_newTileRects = m_newTileRects;
  res->m_newLeafTileRects = m_newLeafTileRects;
  res->m_isEmptyDrawingCoverage = m_isEmptyDrawingCoverage;
  res->m_isEmptyModelAtCoverageCenter = m_isEmptyModelAtCoverageCenter;
  res->m_leafTilesToRender = m_leafTilesToRender;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  res->m_tiles = m_tiles;

  for (TTileSet::const_iterator it = res->m_tiles.begin(); it != res->m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->LockTile(ri);
  }

  tileCache->Unlock();

  res->m_infoLayer.reset();
  res->m_infoLayer.reset(m_infoLayer->clone());
  res->m_stylesCache = 0;

  return res;
}

void ScreenCoverage::SetResourceStyleCache(yg::ResourceStyleCache * stylesCache)
{
  m_stylesCache = stylesCache;
}

yg::ResourceStyleCache * ScreenCoverage::GetResourceStyleCache() const
{
  return m_stylesCache;
}

void ScreenCoverage::Merge(Tiler::RectInfo const & ri)
{
  ASSERT(m_tileRects.find(ri) != m_tileRects.end(), ());

  TileCache & tileCache = m_tileRenderer->GetTileCache();
  TileSet & tileSet = m_tileRenderer->GetTileSet();

  Tile const * tile = 0;
  bool hasTile = false;

  /// every code that works both with tileSet and tileCache
  /// should lock them in the same order to avoid deadlocks
  /// (unlocking should be done in reverse order)
  tileSet.Lock();
  tileCache.Lock();

  hasTile = tileSet.HasTile(ri);

  if (hasTile)
  {
    ASSERT(tileCache.HasTile(ri), ());

    tile = &tileCache.GetTile(ri);
    ASSERT(m_tiles.find(tile) == m_tiles.end(), ());

    /// while in the TileSet, the tile is assumed to be locked

    m_tiles.insert(tile);
    m_tileRects.erase(ri);
    m_newTileRects.erase(ri);
    m_newLeafTileRects.erase(ri);

    if (m_tiler.isLeaf(ri))
    {
      m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;
      m_leafTilesToRender--;
    }

    tileSet.RemoveTile(ri);
  }

  tileCache.Unlock();
  tileSet.Unlock();

  if (hasTile)
  {
    if (m_tiler.isLeaf(ri))
    {
      yg::InfoLayer * tileInfoLayerCopy = tile->m_infoLayer->clone();
      m_infoLayer->merge(*tileInfoLayerCopy,
                          tile->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());

      delete tileInfoLayerCopy;
    }
  }
}

void ScreenCoverage::CacheInfoLayer()
{
  if (!m_stylesCache)
  {
    LOG(LWARNING, ("no styles cache"));
    return;
  }

  m_infoLayer->cache(m_stylesCache);

  /// asserting, that all visible elements is cached
  ASSERT(m_infoLayer->checkCached(m_stylesCache), ());

  m_stylesCache->upload();
}

void ScreenCoverage::Remove(Tile const *)
{
}

void ScreenCoverage::SetScreen(ScreenBase const & screen)
{
  m_screen = screen;

  m_newTileRects.clear();

  m_tiler.seed(m_screen, m_screen.GlobalRect().GlobalCenter());

  vector<Tiler::RectInfo> allRects;
  vector<Tiler::RectInfo> newRects;
  TTileSet tiles;

  m_tiler.tiles(allRects, GetPlatform().PreCachingDepth());

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  m_isEmptyDrawingCoverage = true;
  m_isEmptyModelAtCoverageCenter = true;
  m_leafTilesToRender = 0;

  for (unsigned i = 0; i < allRects.size(); ++i)
  {
    m_tileRects.insert(allRects[i]);
    Tiler::RectInfo ri = allRects[i];

    if (tileCache->HasTile(ri))
    {
      tileCache->TouchTile(ri);
      Tile const * tile = &tileCache->GetTile(ri);
      ASSERT(tiles.find(tile) == tiles.end(), ());

      if (m_tiler.isLeaf(allRects[i]))
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

  TTileSet erasedTiles;
  TTileSet addedTiles;

  set_difference(m_tiles.begin(), m_tiles.end(), tiles.begin(), tiles.end(), inserter(erasedTiles, erasedTiles.end()), TTileSet::key_compare());
  set_difference(tiles.begin(), tiles.end(), m_tiles.begin(), m_tiles.end(), inserter(addedTiles, addedTiles.end()), TTileSet::key_compare());

  for (TTileSet::const_iterator it = erasedTiles.begin(); it != erasedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
    tileCache->UnlockTile((*it)->m_rectInfo);
    /// here we should "unmerge" erasedTiles[i].m_infoLayer from m_infoLayer
  }

  for (TTileSet::const_iterator it = addedTiles.begin(); it != addedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
    tileCache->LockTile((*it)->m_rectInfo);
  }

  tileCache->Unlock();

  m_tiles = tiles;

  MergeInfoLayer();

  set<Tiler::RectInfo> drawnTiles;

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    drawnTiles.insert(Tiler::RectInfo(ri.m_tileScale, ri.m_x, ri.m_y));
  }

  vector<Tiler::RectInfo> firstClassTiles;
  vector<Tiler::RectInfo> secondClassTiles;

  for (unsigned i = 0; i < newRects.size(); ++i)
  {
    Tiler::RectInfo nr = newRects[i];

    Tiler::RectInfo cr[4] =
    {
      Tiler::RectInfo(nr.m_tileScale + 1, nr.m_x * 2,     nr.m_y * 2),
      Tiler::RectInfo(nr.m_tileScale + 1, nr.m_x * 2 + 1, nr.m_y * 2),
      Tiler::RectInfo(nr.m_tileScale + 1, nr.m_x * 2 + 1, nr.m_y * 2 + 1),
      Tiler::RectInfo(nr.m_tileScale + 1, nr.m_x * 2,     nr.m_y * 2 + 1)
    };

    int childTilesToDraw = 4;

    for (int i = 0; i < 4; ++i)
      if (drawnTiles.count(cr[i]) || !m_screen.GlobalRect().IsIntersect(m2::AnyRectD(cr[i].m_rect)))
        --childTilesToDraw;

//    if (m_tiler.isLeaf(nr) || (childTilesToDraw > 1))

    if ((nr.m_tileScale == m_tiler.tileScale() - 2)
      ||(nr.m_tileScale == m_tiler.tileScale() ))
      firstClassTiles.push_back(nr);
    else
      secondClassTiles.push_back(nr);
  }

  /// clearing all old commands
  m_tileRenderer->ClearCommands();
  /// setting new sequenceID
  m_tileRenderer->SetSequenceID(GetSequenceID());

  m_tileRenderer->CancelCommands();

  // filtering out rects that are fully covered by its descedants

  // adding commands for tiles which aren't in cache
  for (size_t i = 0; i < firstClassTiles.size(); ++i)
  {
    Tiler::RectInfo const & ri = firstClassTiles[i];

    core::CommandsQueue::Chain chain;

    chain.addCommand(bind(&CoverageGenerator::AddMergeTileTask,
                          m_coverageGenerator,
                          ri,
                          GetSequenceID()));

    m_tileRenderer->AddCommand(ri, GetSequenceID(),
                               chain);

    if (m_tiler.isLeaf(ri))
      m_newLeafTileRects.insert(ri);

    m_newTileRects.insert(ri);
  }

  for (size_t i = 0; i < secondClassTiles.size(); ++i)
  {
    Tiler::RectInfo const & ri = secondClassTiles[i];

    core::CommandsQueue::Chain chain;

    chain.addCommand(bind(&TileRenderer::RemoveActiveTile,
                          m_tileRenderer,
                          ri));

    m_tileRenderer->AddCommand(ri, GetSequenceID(), chain);
  }
}

ScreenCoverage::~ScreenCoverage()
{
  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->Lock();

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->UnlockTile(ri);
  }

  tileCache->Unlock();
}

void ScreenCoverage::Draw(yg::gl::Screen * s, ScreenBase const & screen)
{
  vector<yg::gl::BlitInfo> infos;

//  LOG(LINFO, ("drawing", m_tiles.size(), "tiles"));

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tile const * tile = *it;

    size_t tileWidth = tile->m_renderTarget->width();
    size_t tileHeight = tile->m_renderTarget->height();

    yg::gl::BlitInfo bi;

    bi.m_matrix = tile->m_tileScreen.PtoGMatrix() * screen.GtoPMatrix();
    bi.m_srcRect = m2::RectI(0, 0, tileWidth - 2, tileHeight - 2);
    bi.m_texRect = m2::RectU(1, 1, tileWidth - 1, tileHeight - 1);
    bi.m_srcSurface = tile->m_renderTarget;

    infos.push_back(bi);
  }

  if (!infos.empty())
    s->blit(&infos[0], infos.size(), true);

  if (m_stylesCache)
  {
    ASSERT(m_infoLayer->checkCached(m_stylesCache), ());
    s->setAdditionalSkinPage(m_stylesCache->cachePage());
  }
  else
    LOG(LINFO, ("no styles cache"));

  m_infoLayer->draw(s, m_screen.PtoGMatrix() * screen.GtoPMatrix());
}

yg::InfoLayer * ScreenCoverage::GetInfoLayer() const
{
  return m_infoLayer.get();
}

void ScreenCoverage::EndFrame(yg::gl::Screen *s)
{
  s->clearAdditionalSkinPage();
}

int ScreenCoverage::GetDrawScale() const
{
  return min(m_tiler.tileScale(), scales::GetUpperScale());
}

bool ScreenCoverage::IsEmptyDrawingCoverage() const
{
  return (m_leafTilesToRender <= 0) && m_isEmptyDrawingCoverage;
}

bool ScreenCoverage::IsEmptyModelAtCoverageCenter() const
{
  return m_isEmptyModelAtCoverageCenter;
}

void ScreenCoverage::CheckEmptyModelAtCoverageCenter()
{
  if (!IsPartialCoverage() && IsEmptyDrawingCoverage())
    m_isEmptyModelAtCoverageCenter = m_coverageGenerator->IsEmptyModelAtPoint(m_screen.GlobalRect().GetGlobalRect().Center());
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

  for (vector<Tile const *>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->UnlockTile(ri);
    m_tiles.erase(*it);
    m_tileRects.erase(ri);
  }

  MergeInfoLayer();
}

void ScreenCoverage::MergeInfoLayer()
{
  m_infoLayer->clear();

  for (TTileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    if (m_tiler.isLeaf(ri))
    {
      scoped_ptr<yg::InfoLayer> copy((*it)->m_infoLayer->clone());
      m_infoLayer->merge(*copy.get(), (*it)->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());
    }
  }
}
