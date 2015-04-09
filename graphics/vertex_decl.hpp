#pragma once

#include "graphics/defines.hpp"
#include "graphics/vertex_stream.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"

namespace graphics
{
  struct VertexStream;
  /// Single attribute of vertex.
  struct VertexAttrib
  {
    ESemantic m_semantic;
    size_t m_offset;
    EDataType m_elemType;
    size_t m_elemCount;
    size_t m_stride;

    VertexAttrib(ESemantic semantic,
                 size_t offset,
                 EDataType elemType,
                 size_t elemCount,
                 size_t stride);

    void initStream(VertexStream * vs,
                    unsigned char * v,
                    unsigned stride) const;
  };

  /// Vertex structure declaration.
  class VertexDecl
  {
  private:
    vector<VertexAttrib> m_attrs;
    size_t m_elemSize;
  public:
    /// constructor.
    VertexDecl(VertexAttrib const * attrs, size_t cnt);
    /// get the number of attributes.
    size_t attrCount() const;
    /// get vertex attribute.
    VertexAttrib const * getAttr(size_t i) const;
    /// size of single element.
    size_t elemSize() const;
    /// initialize vertex stream
    void initStream(VertexStream * vs, unsigned char * v) const;
  };
}
