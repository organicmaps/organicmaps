#pragma once

#include "graphics/opengl/opengl.hpp"

namespace graphics
{
  namespace gl
  {
    namespace glsl
    {
      void glEnable(GLenum cap);
      void glDisable(GLenum cap);
      void glAlphaFunc(GLenum func, GLclampf ref);

      void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
      void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
      void glNormalPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
      void glEnableClientState(GLenum array);

      void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
      void glOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
      void glLoadIdentity();
      void glLoadMatrixf(GLfloat const * data);
      void glMatrixMode(GLenum mode);

      void glUseSharpGeometry(GLboolean flag);
    }
  }
}
