#pragma once

namespace graphics
{
  template <typename T>
  struct PosField
  {
    T * m_x;
    unsigned short m_xStride;

    T * m_y;
    unsigned short m_yStride;

    T * m_z;
    unsigned short m_zStride;

    T * m_w;
    unsigned short m_wStride;

    PosField() : m_x(0),
                 m_xStride(0),
                 m_y(0),
                 m_yStride(0),
                 m_z(0),
                 m_zStride(0),
                 m_w(0),
                 m_wStride(0)
    {}

    template <typename U>
    void copyFrom(PosField<U> const & u)
    {
      if (m_x && u.m_x)
        *m_x = static_cast<T>(*u.m_x);
      if (m_y && u.m_y)
        *m_y = static_cast<T>(*u.m_y);
      if (m_z && u.m_z)
        *m_z = static_cast<T>(*u.m_z);
      if (m_w && u.m_w)
        *m_w = static_cast<T>(*u.m_w);
    }

    void advance(unsigned cnt)
    {
      if (m_x)
        m_x = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_x) + m_xStride * cnt);
      if (m_y)
        m_y = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_y) + m_yStride * cnt);
      if (m_z)
        m_z = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_z) + m_zStride * cnt);
      if (m_w)
        m_w = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_w) + m_wStride * cnt);
    }
  };

  template <typename T>
  struct TexField
  {
    T * m_u;
    unsigned short m_uStride;

    T * m_v;
    unsigned short m_vStride;

    TexField() : m_u(0),
                 m_uStride(0),
                 m_v(0),
                 m_vStride(0)
    {}

    template <typename U>
    void copyFrom(TexField<U> const & u)
    {
      if (m_u && u.m_u)
        *m_u = static_cast<T>(*u.m_u);
      if (m_v && u.m_v)
        *m_v = static_cast<T>(*u.m_v);
    }

    void advance(unsigned cnt)
    {
      if (m_u)
        m_u = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_u) + m_uStride * cnt);
      if (m_v)
        m_v = reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(m_v) + m_vStride * cnt);
    }
  };

  struct VertexStream
  {
    PosField<float> m_fPos;
    PosField<double> m_dPos;
    PosField<float> m_fNormal;
    PosField<double> m_dNormal;
    TexField<float> m_fTex;
    TexField<double> m_dTex;

    /// should be inline for max performance
    inline void copyVertex(VertexStream * dstVS)
    {
      m_fPos.copyFrom(dstVS->m_fPos);
      m_fPos.copyFrom(dstVS->m_dPos);
      m_dPos.copyFrom(dstVS->m_fPos);
      m_dPos.copyFrom(dstVS->m_dPos);

      m_fNormal.copyFrom(dstVS->m_fNormal);
      m_fNormal.copyFrom(dstVS->m_dNormal);
      m_dNormal.copyFrom(dstVS->m_fNormal);
      m_dNormal.copyFrom(dstVS->m_dNormal);

      m_fTex.copyFrom(dstVS->m_fTex);
      m_fTex.copyFrom(dstVS->m_dTex);
      m_dTex.copyFrom(dstVS->m_fTex);
      m_dTex.copyFrom(dstVS->m_dTex);
    }
    /// should be inline for max performance
    inline void advanceVertex(unsigned cnt)
    {
      m_fPos.advance(cnt);
      m_dPos.advance(cnt);
      m_fNormal.advance(cnt);
      m_dNormal.advance(cnt);
      m_fTex.advance(cnt);
      m_dTex.advance(cnt);
    }
  };
}
