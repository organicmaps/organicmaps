#pragma once

#include "tiler.hpp"

#include "geometry/screenbase.hpp"

#include "std/shared_ptr.hpp"

namespace graphics
{
  namespace gl
  {
    class BaseTexture;
  }
  class OverlayStorage;
}

struct Tile
{
  shared_ptr<graphics::gl::BaseTexture> m_renderTarget; //< taken from resource manager
  shared_ptr<graphics::OverlayStorage> m_overlay;  //< text and POI's
  ScreenBase m_tileScreen; //< cached to calculate it once, cause tile blitting
                           //< is performed on GUI thread.
  Tiler::RectInfo m_rectInfo; //< taken from tiler
  bool m_isEmptyDrawing; //< does this tile contains only coasts and oceans
  int m_sequenceID; // SequenceID in witch tile was rendered

  Tile();

  Tile(shared_ptr<graphics::gl::BaseTexture> const & renderTarget,
       shared_ptr<graphics::OverlayStorage> const & overlay,
       ScreenBase const & tileScreen,
       Tiler::RectInfo const & rectInfo,
       bool isEmptyDrawing,
       int sequenceID);

  ~Tile();
};

struct LessRectInfo
{
  bool operator()(Tile const * l, Tile const * r) const;
};
