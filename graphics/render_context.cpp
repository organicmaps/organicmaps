#include "base/SRC_FIRST.hpp"
#include "graphics/render_context.hpp"

namespace graphics
{
  void RenderContext::setMatrix(EMatrix mt, TMatrix const & m)
  {
    m_matrices[mt] = m;
  }

  RenderContext::TMatrix const & RenderContext::matrix(EMatrix m) const
  {
    map<EMatrix, TMatrix >::const_iterator it = m_matrices.find(m);
    return it->second;
  }

  void RenderContext::startThreadDrawing(unsigned threadSlot)
  {
    ASSERT(m_threadSlot == -1, ());
    m_threadSlot = threadSlot;
  }

  void RenderContext::endThreadDrawing(unsigned threadSlot)
  {
    ASSERT(m_threadSlot != -1, ());
    m_threadSlot = -1;
  }

  RenderContext::RenderContext()
    : m_threadSlot(-1)
  {
    setMatrix(EModelView, math::Identity<float, 4>());
    setMatrix(EProjection, math::Identity<float, 4>());
  }

  RenderContext::~RenderContext()
  {}

  void RenderContext::setResourceManager(const shared_ptr<ResourceManager> &rm)
  {
    m_resourceManager = rm;
  }

  shared_ptr<ResourceManager> const & RenderContext::resourceManager() const
  {
    return m_resourceManager;
  }

  unsigned RenderContext::threadSlot() const
  {
    return m_threadSlot;
  }
}
