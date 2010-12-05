#include "memento.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    Memento::Memento()
    {
      m_isDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
      OGLCHECK(glGetIntegerv(GL_DEPTH_FUNC, &m_depthFunc));

      OGLCHECK(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBinding));
      OGLCHECK(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &m_elementArrayBinding));

      OGLCHECK(glGetBooleanv(GL_TEXTURE_2D, (GLboolean*)&m_isTextureEnabled));
      OGLCHECK(glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_level0Texture));

      OGLCHECK(glGetBooleanv(GL_VERTEX_ARRAY, (GLboolean*)&m_isVertexArrayEnabled));
      OGLCHECK(glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &m_vertexArrayType));
      OGLCHECK(glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &m_vertexArrayStride));
      OGLCHECK(glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &m_vertexArraySize));
      OGLCHECK(glGetPointerv(GL_VERTEX_ARRAY_POINTER, &m_vertexArrayPointer));

      OGLCHECK(glGetBooleanv(GL_COLOR_ARRAY, (GLboolean*)&m_isColorArrayEnabled));
      OGLCHECK(glGetIntegerv(GL_COLOR_ARRAY_TYPE, &m_colorArrayType));
      OGLCHECK(glGetIntegerv(GL_COLOR_ARRAY_STRIDE, &m_colorArrayStride));
      OGLCHECK(glGetIntegerv(GL_COLOR_ARRAY_SIZE, &m_colorArraySize));
      OGLCHECK(glGetPointerv(GL_COLOR_ARRAY_POINTER, &m_colorArrayPointer));

      OGLCHECK(glGetBooleanv(GL_TEXTURE_COORD_ARRAY, (GLboolean*)&m_isTexCoordArrayEnabled));
      OGLCHECK(glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &m_texCoordArrayType));
      OGLCHECK(glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE, &m_texCoordArrayStride));
      OGLCHECK(glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &m_texCoordArraySize));
      OGLCHECK(glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &m_texCoordArrayPointer));
    }

    void Memento::apply()
    {
      if (m_isDepthTestEnabled)
        OGLCHECK(glEnable(GL_DEPTH_TEST));
      else
        OGLCHECK(glDisable(GL_DEPTH_TEST));

      OGLCHECK(glDepthFunc(m_depthFunc));

      OGLCHECK(glBindBuffer(GL_ARRAY_BUFFER, m_arrayBinding));
      OGLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBinding));

      if (m_isTextureEnabled)
        OGLCHECK(glEnable(GL_TEXTURE_2D));
      else
        OGLCHECK(glDisable(GL_TEXTURE_2D));

      OGLCHECK(glBindTexture(GL_TEXTURE_2D, m_level0Texture));

      if (m_isVertexArrayEnabled)
      {
        OGLCHECK(glEnableClientState(GL_VERTEX_ARRAY));
        OGLCHECK(glVertexPointer(m_vertexArraySize, m_vertexArrayType, m_vertexArrayStride, m_vertexArrayPointer));
      }
      else
        OGLCHECK(glDisableClientState(GL_VERTEX_ARRAY));

      if (m_isColorArrayEnabled)
      {
        OGLCHECK(glEnableClientState(GL_COLOR_ARRAY));
        OGLCHECK(glColorPointer(m_colorArraySize, m_colorArrayType, m_colorArrayStride, m_colorArrayPointer));
      }
      else
        OGLCHECK(glDisableClientState(GL_COLOR_ARRAY));

      if (m_isTexCoordArrayEnabled)
      {
        OGLCHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        OGLCHECK(glTexCoordPointer(m_texCoordArraySize, m_texCoordArrayType, m_texCoordArrayStride, m_texCoordArrayPointer));
      }
      else
        OGLCHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

    }
  }
}
