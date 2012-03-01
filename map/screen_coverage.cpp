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
    m_leavesCount(0),
    m_stylesCache(0)
{
  m_infoLayer->setCouldOverlap(false);
}

ScreenCoverage::ScreenCoverage(TileRenderer * tileRenderer,
                               CoverageGenerator * coverageGenerator)
  : m_tileRenderer(tileRenderer),
    m_infoLayer(new yg::InfoLayer()),
    m_isEmptyDrawingCoverage(false),
    m_leavesCount(0),
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
  res->m_leavesCount = m_leavesCount;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->writeLock();

  res->m_tiles = m_tiles;

  for (TileSet::const_iterator it = res->m_tiles.begin(); it != res->m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->lockTile(ri);
  }

  tileCache->writeUnlock();

  res->m_infoLayer.reset();
  res->m_infoLayer.reset(m_infoLayer->clone());
  res->m_stylesCache = 0;

  return res;
}

void ScreenCoverage::SetStylesCache(yg::StylesCache * stylesCache)
{
  m_stylesCache = stylesCache;
}

yg::StylesCache * ScreenCoverage::GetStylesCache() const
{
  return m_stylesCache;
}

void ScreenCoverage::Merge(Tiler::RectInfo const & ri)
{
  if (m_tileRects.find(ri) == m_tileRects.end())
    return;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  Tile const * tile = 0;
  bool hasTile = false;

  tileCache->readLock();

  hasTile = tileCache->hasTile(ri);
  if (hasTile)
    tile = &tileCache->getTile(ri);

  bool addTile = false;

  if (hasTile)
  {
    addTile = (m_tiles.find(tile) == m_tiles.end());

    if (addTile)
    {
       tileCache->lockTile(ri);
       m_tiles.insert(tile);
       m_tileRects.erase(ri);
       m_newTileRects.erase(ri);
       m_newLeafTileRects.erase(ri);

       if (m_tiler.isLeaf(ri))
       {
         m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;
         m_leavesCount--;
       }
    }
  }

  tileCache->readUnlock();

  if (addTile)
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

bool LessRectInfo::operator()(Tile const * l, Tile const * r) const
{
  return l->m_rectInfo < r->m_rectInfo;
}

void ScreenCoverage::SetScreen(ScreenBase const & screen)
{
  m_screen = screen;

  m_newTileRects.clear();

  m_tiler.seed(m_screen, m_screen.GlobalRect().GlobalCenter());

  vector<Tiler::RectInfo> allRects;
  vector<Tiler::RectInfo> newRects;
  TileSet tiles;

  m_tiler.tiles(allRects, GetPlatform().PreCachingDepth());

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->writeLock();

  m_isEmptyDrawingCoverage = true;
  m_leavesCount = 0;

  for (unsigned i = 0; i < allRects.size(); ++i)
  {
    m_tileRects.insert(allRects[i]);
    Tiler::RectInfo ri = allRects[i];

    if (tileCache->hasTile(ri))
    {
      tileCache->touchTile(ri);
      Tile const * tile = &tileCache->getTile(ri);
      ASSERT(tiles.find(tile) == tiles.end(), ());

      if (m_tiler.isLeaf(allRects[i]))
        m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;

      tiles.insert(tile);
    }
    else
    {
      newRects.push_back(ri);
      if (m_tiler.isLeaf(ri))
        ++m_leavesCount;
    }
  }

  tileCache->writeUnlock();

  /// computing difference between current and previous coverage
  /// tiles, that aren't in current coverage are unlocked to allow their deletion from TileCache
  /// tiles, that are new to the current coverage are added into m_tiles and locked in TileCache

  TileSet erasedTiles;
  TileSet addedTiles;

  set_difference(m_tiles.begin(), m_tiles.end(), tiles.begin(), tiles.end(), inserter(erasedTiles, erasedTiles.end()), TileSet::key_compare());
  set_difference(tiles.begin(), tiles.end(), m_tiles.begin(), m_tiles.end(), inserter(addedTiles, addedTiles.end()), TileSet::key_compare());

  tileCache->readLock();

  for (TileSet::const_iterator it = erasedTiles.begin(); it != erasedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
    tileCache->unlockTile((*it)->m_rectInfo);
    /// here we should "unmerge" erasedTiles[i].m_infoLayer from m_infoLayer
  }

  for (TileSet::const_iterator it = addedTiles.begin(); it != addedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
    tileCache->lockTile((*it)->m_rectInfo);
  }

  tileCache->readUnlock();

  m_tiles = tiles;

  MergeInfoLayer();

  set<Tiler::RectInfo> drawnTiles;

  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
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

    if (m_tiler.isLeaf(nr) || (childTilesToDraw > 1))
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

    m_tileRenderer->AddCommand(ri, GetSequenceID(),
                               bind(&CoverageGenerator::AddMergeTileTask, m_coverageGenerator,
                                    ri,
                                    GetSequenceID()));

    if (m_tiler.isLeaf(ri))
      m_newLeafTileRects.insert(ri);

    m_newTileRects.insert(ri);
  }

  for (size_t i = 0; i < secondClassTiles.size(); ++i)
    m_tileRenderer->AddCommand(secondClassTiles[i], GetSequenceID());
}

ScreenCoverage::~ScreenCoverage()
{
  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  /// unlocking tiles from the primary level

  tileCache->writeLock();
  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->unlockTile(ri);
  }

  tileCache->writeUnlock();
}

void ScreenCoverage::Draw(yg::gl::Screen * s, ScreenBase const & screen)
{
  vector<yg::gl::BlitInfo> infos;

//  LOG(LINFO, ("drawing", m_tiles.size(), "tiles"));

  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
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
  return (m_leavesCount <= 0) && m_isEmptyDrawingCoverage;
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

  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;

    if (r.IsIntersect(m2::AnyRectD(ri.m_rect)) && (ri.m_tileScale >= startScale))
      toRemove.push_back(*it);
  }

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  for (vector<Tile const *>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->unlockTile(ri);
    m_tiles.erase(*it);
    m_tileRects.erase(ri);
  }

  MergeInfoLayer();
}

void ScreenCoverage::MergeInfoLayer()
{
  m_infoLayer->clear();

  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    if (m_tiler.isLeaf(ri))
    {
      scoped_ptr<yg::InfoLayer> copy((*it)->m_infoLayer->clone());
      m_infoLayer->merge(*copy.get(), (*it)->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());
    }
  }
}
