#pragma once

#include "geometry/screenbase.hpp"
#include "std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class RenderState;
    class BaseTexture;

    struct RenderStateLayer
    {
      RenderState * m_renderState;

      shared_ptr<BaseTexture> m_frontBuffer;
      ScreenBase m_frontScreen;

      shared_ptr<BaseTexture> m_backBuffer;
      ScreenBase m_backScreen;
    };
  }
}
