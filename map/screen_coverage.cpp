#include "../base/SRC_FIRST.hpp"

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
  : m_tiler(0, 0)
{}

ScreenCoverage::ScreenCoverage(TileRenderer * tileRenderer,
                               CoverageGenerator * coverageGenerator,
                               size_t tileSize,
                               size_t scaleEtalonSize)
  : m_tileRenderer(tileRenderer),
    m_tiler(tileSize, scaleEtalonSize),
    m_coverageGenerator(coverageGenerator)
{}

ScreenCoverage::ScreenCoverage(ScreenCoverage const & src)
  : m_tileRenderer(src.m_tileRenderer),
    m_tiler(src.m_tiler),
    m_screen(src.m_screen),
    m_tiles(src.m_tiles),
    m_infoLayer(src.m_infoLayer)
{
  TileCache * tileCache = &m_tileRenderer->GetTileCache();
  tileCache->readLock();

  for (size_t i = 0; i < m_tiles.size(); ++i)
    tileCache->lockTile(m_tiles[i]->m_rectInfo);

  tileCache->readUnlock();
}

ScreenCoverage const & ScreenCoverage::operator=(ScreenCoverage const & src)
{
  if (&src != this)
  {
    m_tileRenderer = src.m_tileRenderer;
    m_tiler = src.m_tiler;
    m_screen = src.m_screen;
    m_tiles = src.m_tiles;

    TileCache * tileCache = &m_tileRenderer->GetTileCache();

    tileCache->readLock();

    for (size_t i = 0; i < m_tiles.size(); ++i)
      tileCache->lockTile(m_tiles[i]->m_rectInfo);

    tileCache->readUnlock();

    m_infoLayer = src.m_infoLayer;
  }
  return *this;
}

void ScreenCoverage::Merge(Tiler::RectInfo const & ri)
{
  threads::MutexGuard g(m_mutex);
  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  Tile const * tile = 0;
  bool hasTile = false;

  tileCache->readLock();
  hasTile = tileCache->hasTile(ri);
  if (hasTile)
    tile = &tileCache->getTile(ri);
  tileCache->readUnlock();

  if (hasTile)
  {
    m_tiles.push_back(tile);
    m_infoLayer.merge(*tile->m_infoLayer.get(),
                      tile->m_tileScreen.PtoGMatrix() * m_screen.GtoPMatrix());
  }
}

void ScreenCoverage::Remove(Tile const * tile)
{
  threads::MutexGuard g(m_mutex);
}

struct lessRectInfo
{
  bool operator()(Tile const * l, Tile const * r)
  {
    return l->m_rectInfo.toUInt64Cell() < r->m_rectInfo.toUInt64Cell();
  }
};

void ScreenCoverage::SetScreen(ScreenBase const & screen, bool mergePathNames)
{
  threads::MutexGuard g(m_mutex);

  m_screen = screen;

  m_tiler.seed(m_screen, m_screen.GlobalRect().Center());

  vector<Tiler::RectInfo> allRects;
  vector<Tiler::RectInfo> newRects;
  vector<Tile const *> tiles;

  m_tiler.visibleTiles(allRects);

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->readLock();

  for (unsigned i = 0; i < allRects.size(); ++i)
  {
    Tiler::RectInfo ri = allRects[i];
    if (tileCache->hasTile(ri))
    {
      tiles.push_back(&tileCache->getTile(ri));
      tileCache->touchTile(ri);
    }
    else
      newRects.push_back(ri);
  }

  /// computing difference between current and previous coverage
  /// tiles, that aren't in current coverage are unlocked to allow their deletion from TileCache
  /// tiles, that are new to the current coverage are added into m_tiles and locked in TileCache

  typedef set<Tile const *, lessRectInfo> TileSet;

  LOG(LINFO, ("prevSet size: ", m_tiles.size()));

  TileSet prevSet;
  copy(m_tiles.begin(), m_tiles.end(), inserter(prevSet, prevSet.end()));

  LOG(LINFO, ("curSet size: ", tiles.size()));

  TileSet curSet;
  copy(tiles.begin(), tiles.end(), inserter(curSet, curSet.end()));

  TileSet erasedTiles;
  TileSet addedTiles;

  set_difference(prevSet.begin(), prevSet.end(), curSet.begin(), curSet.end(), inserter(erasedTiles, erasedTiles.end()));
  set_difference(curSet.begin(), curSet.end(), prevSet.begin(), prevSet.end(), inserter(addedTiles, addedTiles.end()));

  LOG(LINFO, ("erased tiles size: ", erasedTiles.size()));

  for (TileSet::const_iterator it = erasedTiles.begin(); it != erasedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
//    LOG(LINFO, ("erasing tile: ", ri.m_tileScale, ri.m_drawScale, ri.m_y, ri.m_x));
    tileCache->unlockTile((*it)->m_rectInfo);
    /// here we should "unmerge" erasedTiles[i].m_infoLayer from m_infoLayer
  }

  LOG(LINFO, ("added tiles size: ", addedTiles.size()));

  for (TileSet::const_iterator it = addedTiles.begin(); it != addedTiles.end(); ++it)
  {
    Tiler::RectInfo ri = (*it)->m_rectInfo;
//    LOG(LINFO, ("adding tile: ", ri.m_tileScale, ri.m_drawScale, ri.m_y, ri.m_x));
    tileCache->lockTile((*it)->m_rectInfo);
//    m_infoLayer.merge(*((*it)->m_infoLayer.get()), (*it)->m_tileScreen.PtoGMatrix() * screen.GtoPMatrix());
  }

  m_infoLayer.clear();
  for (size_t i = 0; i < tiles.size(); ++i)
    m_infoLayer.merge(*tiles[i]->m_infoLayer.get(), tiles[i]->m_tileScreen.PtoGMatrix() * screen.GtoPMatrix());

  tileCache->readUnlock();

  m_tiles = tiles;

  /// adding commands for tiles which aren't in cache
  for (size_t i = 0; i < newRects.size(); ++i)
  {
    m_tileRenderer->AddCommand(newRects[i], m_tiler.sequenceID(),
                               bind(&CoverageGenerator::AddMergeTileTask, m_coverageGenerator, newRects[i]));
  }
}

ScreenCoverage::~ScreenCoverage()
{
  Clear();
}

void ScreenCoverage::Clear()
{
  threads::MutexGuard g(m_mutex);

  TileCache * tileCache = &m_tileRenderer->GetTileCache();

  tileCache->readLock();
  for (unsigned i = 0; i < m_tiles.size(); ++i)
    tileCache->unlockTile(m_tiles[i]->m_rectInfo);

  tileCache->readUnlock();

  m_tiles.clear();
  m_infoLayer.clear();
}

void ScreenCoverage::Draw(yg::gl::Screen * s, ScreenBase const & screen)
{
  threads::MutexGuard g(m_mutex);

  for (size_t i = 0; i < m_tiles.size(); ++i)
  {
    Tile const * tile = m_tiles[i];
    size_t tileWidth = tile->m_renderTarget->width();
    size_t tileHeight = tile->m_renderTarget->height();

    s->blit(tile->m_renderTarget, tile->m_tileScreen, screen, true,
            yg::Color(),
            m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
            m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));

  }

  m_infoLayer.draw(s, m_screen.PtoGMatrix() * screen.GtoPMatrix());
}
