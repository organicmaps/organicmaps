#include "vertex_decl.hpp"
#include "defines.hpp"

namespace graphics
{
  VertexAttrib::VertexAttrib(ESemantic semantic,
                             size_t offset,
                             EDataType elemType,
                             size_t elemCount,
                             size_t stride)
    : m_semantic(semantic),
      m_offset(offset),
      m_elemType(elemType),
      m_elemCount(elemCount),
      m_stride(stride)
  {}

  VertexDecl::VertexDecl(VertexAttrib const * attrs, size_t cnt)
  {
    copy(attrs, attrs + cnt, back_inserter(m_attrs));

    m_elemSize = 0;

    for (unsigned i = 0; i < m_attrs.size(); ++i)
    {
      VertexAttrib & va = m_attrs[i];
      m_elemSize += graphics::elemSize(va.m_elemType) * va.m_elemCount;
    }
  }

  VertexAttrib const * VertexDecl::getAttr(size_t i) const
  {
    return &m_attrs[i];
  }

  size_t VertexDecl::attrCount() const
  {
    return m_attrs.size();
  }

  size_t VertexDecl::elemSize() const
  {
    return m_elemSize;
  }
}
