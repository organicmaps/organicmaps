#include "../base/SRC_FIRST.hpp"
#include "render_context.hpp"

namespace graphics
{
  void RenderContext::setMatrix(EMatrix mt, math::Matrix<float, 4, 4> const & m)
  {
    m_matrices[mt] = m;
  }

  math::Matrix<float, 4, 4> const & RenderContext::matrix(EMatrix m) const
  {
    map<EMatrix, math::Matrix<float, 4, 4> >::const_iterator it = m_matrices.find(m);
    return it->second;
  }

  RenderContext::RenderContext()
  {
    setMatrix(EModelView, math::Identity<float, 4>());
    setMatrix(EProjection, math::Identity<float, 4>());
  }

  RenderContext::~RenderContext()
  {}
}
