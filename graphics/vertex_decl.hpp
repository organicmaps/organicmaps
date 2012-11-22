#pragma once

#include "defines.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

namespace graphics
{
  /// Single attribute of vertex.
  struct VertexAttrib
  {
    size_t m_offset;
    EDataType m_elemType;
    size_t m_elemCount;
    size_t m_stride;
    string m_name;

    VertexAttrib(size_t offset,
                 EDataType elemType,
                 size_t elemCount,
                 size_t stride);
  };

  /// Vertex structure declaration.
  class VertexDecl
  {
  private:
    vector<VertexAttrib> m_attrs;
  public:
    /// constructor.
    VertexDecl(VertexAttrib const * attrs, size_t cnt);
    /// get the number of attributes.
    size_t size() const;
    /// get vertex attribute.
    VertexAttrib const * getAttr(size_t i) const;
  };
}
