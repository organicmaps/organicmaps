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

  /// Queue to put new rendering tasks in
  TileRenderer * m_tileRenderer;
  /// Tiler to compute visible and predicted tiles
  Tiler m_tiler;
  /// Last covered screen
  ScreenBase  m_screen;
  /// Container for a rects, that forms a set of tiles in the m_screen
  typedef set<Tiler::RectInfo> TileRectSet;
  /// All rects, including rects which corresponds to a tiles in m_tiles
  TileRectSet m_tileRects;
  /// Only rects, that should be drawn
  TileRectSet m_newTileRects;
  /// Typedef for a set of tiles, that are visible for the m_screen
  typedef set<Tile const *, LessRectInfo> TileSet;
  TileSet m_tiles;
  /// InfoLayer composed of infoLayers for visible tiles
  scoped_ptr<yg::InfoLayer> m_infoLayer;
  /// Primary scale, which is used to draw tiles in m_screen.
  /// Not all tiles could correspond to this value, as there could be tiles from
  /// lower and higher level in the coverage to provide a smooth
  /// scale transition experience
  int m_drawScale;

  bool m_isEmptyDrawingCoverage;

  CoverageGenerator * m_coverageGenerator;
  yg::StylesCache * m_stylesCache;

  ScreenCoverage(ScreenCoverage const & src);
  ScreenCoverage const & operator=(ScreenCoverage const & src);

public:

  ScreenCoverage();
  ~ScreenCoverage();
  ScreenCoverage(TileRenderer * tileRenderer,
                 CoverageGenerator * coverageGenerator,
                 size_t tileSize,
                 size_t scaleEtalonSize);

  ScreenCoverage * Clone();
  /// Is this screen coverage partial, which means that it contains non-drawn rects
  bool IsPartialCoverage() const;
  /// Is this screen coverage contains only empty tiles
  bool IsEmptyDrawingCoverage() const;
  /// Setters/Getters for current stylesCache
  void SetStylesCache(yg::StylesCache * stylesCache);
  yg::StylesCache * GetStylesCache() const;
  /// Getter for InfoLayer
  yg::InfoLayer * GetInfoLayer() const;
  /// Cache info layer on current style cache
  void CacheInfoLayer();
  /// add rendered tile to coverage. Tile is locked, so make sure to unlock it in case it's not needed.
  void Merge(Tiler::RectInfo const & ri);
  /// remove tile from coverage
  void Remove(Tile const * tile);
  /// recalculate screen coverage, using as much info from prev coverage as possible
  void SetScreen(ScreenBase const & screen);
  /// draw screen coverage
  void Draw(yg::gl::Screen * s, ScreenBase const & currentScreen);
  /// perform end frame
  void EndFrame(yg::gl::Screen * s);
  /// get draw scale for the tiles in the current coverage
  int GetDrawScale() const;
};
