#pragma once

#include "tiler.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class BaseTexture;
  }
  class InfoLayer;
}

struct Tile
{
  shared_ptr<yg::gl::BaseTexture> m_renderTarget; //< taken from resource manager
  shared_ptr<yg::InfoLayer> m_infoLayer;  //< text and POI's
  ScreenBase m_tileScreen; //< cached to calculate it once, cause tile blitting
                           //< is performed on GUI thread.
  Tiler::RectInfo m_rectInfo; //< taken from tiler
  double m_duration; //< how long does it take to render tile
  bool m_isEmptyDrawing; //< does this tile contains only coasts and oceans

  Tile();

  Tile(shared_ptr<yg::gl::BaseTexture> const & renderTarget,
       shared_ptr<yg::InfoLayer> const & infoLayer,
       ScreenBase const & tileScreen,
       Tiler::RectInfo const & rectInfo,
       double duration,
       bool isEmptyDrawing);

  ~Tile();
};

struct LessRectInfo
{
  bool operator()(Tile const * l, Tile const * r) const;
};

