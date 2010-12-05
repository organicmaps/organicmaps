#pragma once

#include "../base/mutex.hpp"
#include "../std/function.hpp"
#include "../std/list.hpp"
#include "../std/shared_ptr.hpp"
#include "../geometry/screenbase.hpp"
#include "texture.hpp"

namespace yg
{
  namespace gl
  {
    class RenderBuffer;

    class RenderState
    {
    public:

      typedef function<void()> invalidateFn;

      shared_ptr<BaseTexture> m_backBuffer;
      shared_ptr<RenderBuffer> m_depthBuffer;

      /// Already rendered model params
      /// @{
      /// Bitmap
      shared_ptr<BaseTexture> m_actualTarget;
      /// Screen parameters
      ScreenBase m_actualScreen;
      /// @}

      /// Screen of the rendering operation in progress.
      ScreenBase m_currentScreen;
      /// Duration of the rendering operation
      double m_duration;

      /// Surface height and width.
      unsigned int m_surfaceWidth;
      unsigned int m_surfaceHeight;

      /// Texture height and width.
      unsigned int m_textureWidth;
      unsigned int m_textureHeight;

      /// Have this state been resized?
      bool m_isResized;
      /// RepaintAll flag
      bool m_doRepaintAll;

      mutable shared_ptr<threads::Mutex> m_mutex;

/*      RenderState(shared_ptr<RGBA8Texture> const & actualTarget,
                  ScreenBase actualScreen);
 */

      list<invalidateFn> m_invalidateFns;

      RenderState();

      bool isPanning() const;

      void copyTo(RenderState & s) const;

      void invalidate();

      void addInvalidateFn(invalidateFn fn);

      void onSize(size_t w, size_t h);
    };
  }
}
