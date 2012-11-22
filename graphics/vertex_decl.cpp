#include "vertex_decl.hpp"

namespace graphics
{
  VertexAttrib::VertexAttrib(size_t offset,
                             EDataType elemType,
                             size_t elemCount,
                             size_t stride)
    : m_offset(offset),
      m_elemType(elemType),
      m_elemCount(elemCount),
      m_stride(stride)
  {}

  VertexDecl::VertexDecl(VertexAttrib const * attrs, size_t cnt)
  {
    copy(attrs, attrs + cnt, back_inserter(m_attrs));
  }

  VertexAttrib const * VertexDecl::getAttr(size_t i) const
  {
    return &m_attrs[i];
  }

  size_t VertexDecl::size() const
  {
    return m_attrs.size();
  }
}
