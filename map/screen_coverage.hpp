#pragma once

#include "../base/mutex.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/info_layer.hpp"

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

  threads::Mutex m_mutex;

  TileRenderer * m_tileRenderer; //< queue to put new rendering tasks in
  Tiler m_tiler; //< tiler to compute visible and predicted tiles

  ScreenBase  m_screen; //< last covered screen
  vector<Tile const * > m_tiles; //< vector of visible tiles for m_screen
  yg::InfoLayer m_infoLayer; //< composite infoLayers for visible tiles

  CoverageGenerator * m_coverageGenerator;

public:

  ScreenCoverage();
  ScreenCoverage(TileRenderer * tileRenderer, CoverageGenerator * coverageGenerator, size_t tileSize, size_t scaleEtalonSize);
  ~ScreenCoverage();

  ScreenCoverage(ScreenCoverage const & src);
  ScreenCoverage const & operator=(ScreenCoverage const & src);

  /// add rendered tile to coverage. Tile is locked, so make sure to unlock it in case it's not needed.
  void Merge(Tiler::RectInfo const & ri);
  /// remove tile from coverage
  void Remove(Tile const * tile);
  /// recalculate screen coverage, using as much info from prev coverage as possible
  void SetScreen(ScreenBase const & screen, bool mergePathNames = true);
  /// clear screen coverage and associated info layer
  void Clear();
  /// draw screen coverage
  void Draw(yg::gl::Screen * s, ScreenBase const & currentScreen);
};
