#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"
#include "../std/set.hpp"
#include "../std/algorithm.hpp"

#include "../yg/screen.hpp"
#include "../yg/base_texture.hpp"

#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "window_handle.hpp"
#include "coverage_generator.hpp"

ScreenCoverage::ScreenCoverage()
  : m_tiler(0, 0),
    m_infoLayer(new yg::InfoLayer()),
    m_drawScale(0),
    m_isEmptyDrawingCoverage(false),
    m_stylesCache(0)
{
  m_infoLayer->setCouldOverlap(false);
}

ScreenCoverage::ScreenCoverage(TileRenderer * tileRenderer,
                               CoverageGenerator * coverageGenerator,
                               size_t tileSize,
                               size_t scaleEtalonSize)
  : m_tileRenderer(tileRenderer),
    m_tiler(tileSize, scaleEtalonSize),
    m_infoLayer(new yg::InfoLayer()),
    m_drawScale(0),
    m_isEmptyDrawingCoverage(false),
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
  res->m_drawScale = m_drawScale;
  res->m_isEmptyDrawingCoverage = m_isEmptyDrawingCoverage;

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->writeLock();

  res->m_tiles = m_tiles;

  for (TileSet::const_iterator it = res->m_tiles.begin(); it != res->m_tiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->lockTile(ri);
  }

  res->m_prevTiles = m_prevTiles;

  for (TileSet::const_iterator it = res->m_prevTiles.begin(); it != res->m_prevTiles.end(); ++it)
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

       m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;
    }
  }

  tileCache->readUnlock();

  if (addTile)
  {
    yg::InfoLayer * tileInfoLayerCopy = tile->m_infoLayer->clone();

    m_infoLayer->merge(*tileInfoLayerCopy,
                        tile->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());

    delete tileInfoLayerCopy;
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
  vector<Tiler::RectInfo> allPrevRects;
  vector<Tiler::RectInfo> newRects;
  TileSet tiles;
  TileSet prevTiles;

  m_tiler.currentLevelTiles(allRects);
  m_tiler.prevLevelTiles(allPrevRects, GetPlatform().PreCachingDepth());

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->writeLock();

  m_drawScale = m_tiler.drawScale();

  m_isEmptyDrawingCoverage = true;

  for (unsigned i = 0; i < allRects.size(); ++i)
  {
    m_tileRects.insert(allRects[i]);
    Tiler::RectInfo ri = allRects[i];
    if (tileCache->hasTile(ri))
    {
      tileCache->touchTile(ri);
      Tile const * tile = &tileCache->getTile(ri);
      ASSERT(tiles.find(tile) == tiles.end(), ());

      m_isEmptyDrawingCoverage &= tile->m_isEmptyDrawing;

      tiles.insert(tile);
    }
    else
      newRects.push_back(ri);
  }

  for (TileSet::const_iterator it = m_prevTiles.begin(); it != m_prevTiles.end(); ++it)
    tileCache->unlockTile((*it)->m_rectInfo);

  m_prevTiles.clear();

  for (unsigned i = 0; i < allPrevRects.size(); ++i)
  {
    Tiler::RectInfo ri = allPrevRects[i];
    if (tileCache->hasTile(ri))
    {
      tileCache->touchTile(ri);
      Tile const * tile = &tileCache->getTile(ri);
      ASSERT(m_prevTiles.find(tile) == m_prevTiles.end(), ());

      /// here we lock tiles from the previous level to prevent them from the deletion from cache
      tileCache->lockTile(ri);

      m_prevTiles.insert(tile);
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

  m_infoLayer->clear();

  for (TileSet::const_iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
  {
    yg::InfoLayer * tileInfoLayerCopy = (*it)->m_infoLayer->clone();
    m_infoLayer->merge(*tileInfoLayerCopy, (*it)->m_tileScreen.PtoGMatrix() * screen.GtoPMatrix());
    delete tileInfoLayerCopy;
  }

  copy(newRects.begin(), newRects.end(), inserter(m_newTileRects, m_newTileRects.end()));
  /// clearing all old commands
  m_tileRenderer->ClearCommands();
  /// setting new sequenceID
  m_tileRenderer->SetSequenceID(m_tiler.sequenceID());

  m_tileRenderer->CancelCommands();

  /// adding commands for tiles which aren't in cache
  for (size_t i = 0; i < newRects.size(); ++i)
    m_tileRenderer->AddCommand(newRects[i], m_tiler.sequenceID(),
                               bind(&CoverageGenerator::AddMergeTileTask, m_coverageGenerator, newRects[i]));

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

  /// unlocking tiles from the previous level

  for (TileSet::const_iterator it = m_prevTiles.begin(); it != m_prevTiles.end(); ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache->unlockTile(ri);
  }

  tileCache->writeUnlock();
}

void ScreenCoverage::Draw(yg::gl::Screen * s, ScreenBase const & screen)
{
  vector<yg::gl::BlitInfo> infos;

  for (TileSet::const_iterator it = m_prevTiles.begin(); it != m_prevTiles.end(); ++it)
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
  return m_drawScale;
}

bool ScreenCoverage::IsEmptyDrawingCoverage() const
{
  return m_isEmptyDrawingCoverage;
}

bool ScreenCoverage::IsPartialCoverage() const
{
  return !m_newTileRects.empty();
}
