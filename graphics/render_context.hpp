#pragma once

#include "../../std/shared_ptr.hpp"
#include "../../std/map.hpp"
#include "../../base/matrix.hpp"
#include "defines.hpp"

namespace graphics
{
  namespace gl
  {
    class Program;
    class BaseTexture;
  }

  /// base class for render contexts.
  /// contains current render state data.
  class RenderContext
  {
  private:

    /// Rendering states
    /// @{
    map<EMatrix, math::Matrix<float, 4, 4> > m_matrices;
    /// @}

  public:

    /// Working with rendering states.
    /// @{
    math::Matrix<float, 4, 4> const & matrix(EMatrix m) const;
    void setMatrix(EMatrix mt, math::Matrix<float, 4, 4> const & m);
    /// @}

    /// Constructor.
    RenderContext();
    /// Destructor.
    virtual ~RenderContext();
    /// Make this context current for the specified thread.
    virtual void makeCurrent() = 0;
    /// Create a render context which is shared with this one.
    /// Context sharing means that all resources created in one context
    /// can be used in shared context and vice versa.
    virtual shared_ptr<RenderContext> createShared() = 0;
    /// this function should be called from each opengl thread
    /// to setup some thread parameters.
    virtual void startThreadDrawing() = 0;
    /// called at the end of thread
    virtual void endThreadDrawing() = 0;
  };
}
