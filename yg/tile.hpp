#pragma once

#include "../geometry/screenbase.hpp"
#include "../std/shared_ptr.hpp"
#include "base_texture.hpp"
#include "renderbuffer.hpp"
#include "tiler.hpp"

namespace yg
{
  struct Tile
  {
    shared_ptr<gl::RenderBuffer> m_depthBuffer; //< taken from resource manager
    shared_ptr<gl::BaseTexture> m_renderTarget; //< taken from resource manager
    ScreenBase m_tileScreen;
    Tiler::RectInfo m_rectInfo; //< taken from tiler
  };
}
