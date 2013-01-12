#include "resource.hpp"

#include "opengl/data_traits.hpp"

namespace graphics
{
  Resource::Info::Info(Category cat)
    : m_category(cat)
  {}

  Resource::Resource(
      Category cat,
      m2::RectU const & texRect,
      int pipelineID)
    : m_cat(cat),
    m_texRect(texRect),
    m_pipelineID(pipelineID)
  {}

  Resource::~Resource()
  {}
}
