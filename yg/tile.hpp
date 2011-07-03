#pragma once

#include "../geometry/screenbase.hpp"
#include "../std/shared_ptr.hpp"
#include "base_texture.hpp"
#include "resource_manager.hpp"
#include "tiler.hpp"

namespace yg
{
  struct Tile
  {
    shared_ptr<gl::BaseTexture> m_renderTarget; //< taken from resource manager
    ScreenBase m_tileScreen; //< cached to calculate it once, cause tile blitting
                             //< is performed on GUI thread.
    Tiler::RectInfo m_rectInfo; //< taken from tiler
    double m_duration;

    Tile()
    {}

    Tile(shared_ptr<gl::BaseTexture> const & renderTarget,
         ScreenBase const & tileScreen,
         Tiler::RectInfo const & rectInfo,
         double duration)
      : m_renderTarget(renderTarget),
        m_tileScreen(tileScreen),
        m_rectInfo(rectInfo),
        m_duration(duration)
    {}
  };
}
