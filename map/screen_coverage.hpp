#pragma once

#include "../base/mutex.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/info_layer.hpp"
#include "../yg/resource_style_cache.hpp"

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

class ScreenCoverage
{
private:

  /// Unique ID of this screen coverage
  int m_sequenceID;
  /// Queue to put new rendering tasks in
  TileRenderer * m_tileRenderer;
  /// Tiler to compute visible and predicted tiles
  Tiler m_tiler;
  /// Last covered screen
  ScreenBase  m_screen;
  /// Container for a rects, that forms a set of tiles in the m_screen
  typedef set<Tiler::RectInfo> TTileRectSet;
  /// All rects, including rects which corresponds to a tiles in m_tiles
  TTileRectSet m_tileRects;
  /// Only rects, that should be drawn
  TTileRectSet m_newTileRects;
  /// Only leaf rects, that should be drawn
  TTileRectSet m_newLeafTileRects;
  /// Typedef for a set of tiles, that are visible for the m_screen
  typedef set<Tile const *, LessRectInfo> TTileSet;
  TTileSet m_tiles;
  /// InfoLayer composed of infoLayers for visible tiles
  scoped_ptr<yg::InfoLayer> m_infoLayer;
  /// Does this coverage holds only tiles that are empty
  bool m_isEmptyDrawingCoverage;
  /// Does the model empty at the screen center?
  bool m_isEmptyModelAtCoverageCenter;
  /// How many "leaf" tiles we should render to cover the screen
  int m_leafTilesToRender;

  CoverageGenerator * m_coverageGenerator;
  yg::ResourceStyleCache * m_stylesCache;

  ScreenCoverage(ScreenCoverage const & src);
  ScreenCoverage const & operator=(ScreenCoverage const & src);

  void MergeInfoLayer();

public:

  ScreenCoverage();
  ~ScreenCoverage();
  ScreenCoverage(TileRenderer * tileRenderer,
                 CoverageGenerator * coverageGenerator);

  ScreenCoverage * Clone();

  /// set unique ID for all actions, used to compute this coverage
  void SetSequenceID(int id);
  /// get unique ID for all actions, used to compute this coverage
  int GetSequenceID() const;
  /// Is this screen coverage partial, which means that it contains non-drawn rects
  bool IsPartialCoverage() const;
  /// Is this screen coverage contains only empty tiles
  bool IsEmptyDrawingCoverage() const;
  /// Is the model empty at the screen center
  bool IsEmptyModelAtCoverageCenter() const;
  /// Check, whether the model is empty at the center of the coverage.
  void CheckEmptyModelAtCoverageCenter();
  /// Setters/Getters for current stylesCache
  void SetResourceStyleCache(yg::ResourceStyleCache * stylesCache);
  yg::ResourceStyleCache * GetResourceStyleCache() const;
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
  /// Not all tiles in coverage could correspond to this value,
  /// as there could be tiles from lower and higher level in the
  /// coverage to provide a smooth scale transition experience
  int GetDrawScale() const;
  /// Unlock and remove tiles which intersect the specified rect
  /// and deeper or equal than specified scale
  void RemoveTiles(m2::AnyRectD const & r, int startScale);
};
