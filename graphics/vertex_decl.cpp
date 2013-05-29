#include "vertex_decl.hpp"
#include "defines.hpp"
#include "../base/assert.hpp"
#include "../base/logging.hpp"

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

  template <typename T>
  struct FillPosition
  {
    static void fill(VertexAttrib const * va, PosField<T> * p, unsigned char * v, unsigned stride)
    {
      if (va->m_elemCount > 3)
      {
        p->m_w = reinterpret_cast<T*>(v + sizeof(T) * 3);
        p->m_wStride = stride;
      }
      if (va->m_elemCount > 2)
      {
        p->m_z = reinterpret_cast<T*>(v + sizeof(T) * 2);
        p->m_zStride = stride;
      }
      if (va->m_elemCount > 1)
      {
        p->m_y = reinterpret_cast<T*>(v + sizeof(T) * 1);
        p->m_yStride = stride;
      }
      if (va->m_elemCount > 0)
      {
        p->m_x = reinterpret_cast<T*>(v);
        p->m_xStride = stride;
      }
    }
  };

  template <ESemantic sem>
  struct FillSemantic
  {
    static void fill(VertexAttrib const * va, VertexStream * vs, unsigned char * v);
  };

  template <>
  struct FillSemantic<ESemPosition>
  {
    static void fill(VertexAttrib const * va, VertexStream * vs, unsigned char * v, unsigned stride)
    {
      switch (va->m_elemType)
      {
      case EFloat:
        FillPosition<float>::fill(va, &vs->m_fPos, v, stride);
        break;
      default:
        break;
      }
    }
  };

  template <>
  struct FillSemantic<ESemNormal>
  {
    static void fill(VertexAttrib const * va, VertexStream * vs, unsigned char * v, unsigned stride)
    {
      switch (va->m_elemType)
      {
      case EFloat:
        FillPosition<float>::fill(va, &vs->m_fNormal, v, stride);
        break;
      default:
        ASSERT(false, ("Not supported"));
      }
    }
  };

  template <typename T>
  struct FillTexCoord
  {
    static void fill(VertexAttrib const * va, TexField<T> * p, unsigned char * v, unsigned stride)
    {
      if (va->m_elemCount > 1)
      {
        p->m_v = reinterpret_cast<T*>(v + sizeof(T));
        p->m_vStride = stride;
      }
      if (va->m_elemCount > 0)
      {
        p->m_u = reinterpret_cast<T*>(v);
        p->m_uStride = stride;
      }
    }
  };

  template <>
  struct FillSemantic<ESemTexCoord0>
  {
    static void fill(VertexAttrib const * va, VertexStream * vs, unsigned char * v, unsigned stride)
    {
      switch (va->m_elemType)
      {
      case EFloat:
        FillTexCoord<float>::fill(va, &vs->m_fTex, v, stride);
        break;
      default:
        ASSERT(false, ("Not supported"));
      };
    }
  };

  void VertexAttrib::initStream(VertexStream * vs, unsigned char * v, unsigned stride) const
  {
    switch (m_semantic)
    {
    case ESemPosition:
      FillSemantic<ESemPosition>::fill(this, vs, v, stride);
      break;
    case ESemNormal:
      FillSemantic<ESemNormal>::fill(this, vs, v, stride);
      break;
    case ESemTexCoord0:
      FillSemantic<ESemTexCoord0>::fill(this, vs, v, stride);
      break;
    default:
      LOG(LERROR, ("Unknown semantic specified"));
      break;
    };
  }


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

  void VertexDecl::initStream(VertexStream *vs, unsigned char * v) const
  {
    *vs = VertexStream();
    unsigned char * fieldOffs = v;
    for (size_t i = 0; i < m_attrs.size(); ++i)
    {
      VertexAttrib const * va = &m_attrs[i];
      va->initStream(vs, fieldOffs + va->m_offset, m_elemSize);
    }
  }
}
