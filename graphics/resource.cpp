#include "graphics/resource.hpp"

#include "graphics/opengl/data_traits.hpp"

namespace graphics
{
  bool Resource::LessThan::operator()(Info const * l,
                                      Info const * r) const
  {
    return l->lessThan(r);
  }

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
