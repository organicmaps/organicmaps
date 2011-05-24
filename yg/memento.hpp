#pragma once

namespace yg
{
  namespace gl
  {
    struct Memento
    {
      bool m_isDepthTestEnabled;
      int m_depthFunc;

      bool m_isVertexArrayEnabled;
      int m_vertexArraySize;
      int m_vertexArrayStride;
      int m_vertexArrayType;
      void * m_vertexArrayPointer;

      bool m_isColorArrayEnabled;
      int m_colorArraySize;
      int m_colorArrayStride;
      int m_colorArrayType;
      void * m_colorArrayPointer;

      bool m_isTexCoordArrayEnabled;
      int m_texCoordArraySize;
      int m_texCoordArrayStride;
      int m_texCoordArrayType;
      void * m_texCoordArrayPointer;
      bool m_isTextureEnabled;
      int m_level0Texture;

      int m_arrayBinding;
      int m_elementArrayBinding;

      /// Save current states to memento.
      Memento();
      /// Apply states saved in this memento to the current state
      void apply();
    };

  }
}
