#pragma once

#include "std/shared_ptr.hpp"
#include "std/map.hpp"
#include "base/matrix.hpp"
#include "graphics/defines.hpp"

namespace graphics
{
  class ResourceManager;

  /// base class for render contexts.
  /// contains current render state data.
  class RenderContext
  {
  public:

    typedef math::Matrix<float, 4, 4> TMatrix;

  private:

    /// Rendering states
    /// @{
    map<EMatrix, TMatrix> m_matrices;
    /// @}

    unsigned m_threadSlot;
    shared_ptr<ResourceManager> m_resourceManager;

  protected:

    unsigned threadSlot() const;

  public:

    /// Working with rendering states.
    /// @{
    TMatrix const & matrix(EMatrix m) const;
    void setMatrix(EMatrix mt, TMatrix const & m);
    /// @}

    /// Constructor
    RenderContext();
    /// Destructor.
    virtual ~RenderContext();
    /// Make this context current for the specified thread.
    virtual void makeCurrent() = 0;
    /// Create a render context which is shared with this one.
    /// Context sharing means that all resources created in one context
    /// can be used in shared context and vice versa.
    virtual RenderContext * createShared() = 0;
    /// this function should be called from each opengl thread
    /// to setup some thread parameters.
    virtual void startThreadDrawing(unsigned threadSlot);
    /// called at the end of thread
    virtual void endThreadDrawing(unsigned threadSlot);
    /// set attached resource manager
    void setResourceManager(shared_ptr<ResourceManager> const & rm);
    /// get the attached resource manager
    shared_ptr<ResourceManager> const & resourceManager() const;
  };
}
