#pragma once

#include <internal/opengl.hpp>

namespace yg
{
  namespace gl
  {
    struct ArrayParams
    {
      /// vertexArray params
      bool m_enabled;
      GLint m_size;
      GLenum m_type;
      GLsizei m_stride;
      GLvoid * m_ptr;
    };

    class GPUState
    {
    private:

      int m_readFBO;
      int m_drawFBO;
      int m_fboColor0;
      int m_fboDepth;
      int m_fboStencil;

      int m_tex0;
      int m_vb;
      int m_ib;

      ArrayParams m_vertexAP;
      ArrayParams m_colorAP;
      ArrayParams m_texCoordAP;

      bool m_depthTest;
      GLenum m_depthFunc;

      bool m_alphaTest;
      GLenum m_alphaFunc;
      GLclampf m_ref;

      bool m_alphaBlend;
      GLenum m_srcFactor;
      GLenum m_dstFactor;

      yg::Color m_color;

    public:


    };
  }
}
