#pragma once

#include "../base/mutex.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/info_layer.hpp"
#include "../yg/styles_cache.hpp"

#include "tile.hpp"
#include "tiler.hpp"
#include "render_policy.hpp"

class TileRenderer;

namespace yg
{
  namespace gl
  {
    class Screen;
  }
}

class CoverageGenerator;

struct LessRectInfo
{
  bool operator()(Tile const * l, Tile const * r) const;
};

class ScreenCoverage
{
private:

  TileRenderer * m_tileRenderer; //< queue to put new rendering tasks in
  Tiler m_tiler; //< tiler to compute visible and predicted tiles

  ScreenBase  m_screen; //< last covered screen

  typedef set<Tile const *, LessRectInfo> TileSet;

  typedef set<Tiler::RectInfo> TileRectSet;
  TileRectSet m_tileRects; //< rects, that forms a set of tiles in current rect.

  TileSet m_tiles; //< set of tiles, that are visible for the m_screen
  yg::InfoLayer m_infoLayer; //< composite infoLayers for visible tiles
  shared_ptr<yg::StylesCache> m_stylesCache;

  CoverageGenerator * m_coverageGenerator;

  ScreenCoverage(ScreenCoverage const & src);
  ScreenCoverage const & operator=(ScreenCoverage const & src);

public:

  ScreenCoverage();
  ScreenCoverage(TileRenderer * tileRenderer, CoverageGenerator * coverageGenerator, size_t tileSize, size_t scaleEtalonSize);
  ~ScreenCoverage();

  ScreenCoverage * Clone();

  /// add rendered tile to coverage. Tile is locked, so make sure to unlock it in case it's not needed.
  void Merge(Tiler::RectInfo const & ri);
  /// remove tile from coverage
  void Remove(Tile const * tile);
  /// recalculate screen coverage, using as much info from prev coverage as possible
  void SetScreen(ScreenBase const & screen, bool mergePathNames = true);
  /// draw screen coverage
  void Draw(yg::gl::Screen * s, ScreenBase const & currentScreen);
};
