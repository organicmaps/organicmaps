#pragma once

#include "opengl.hpp"
#include "storage.hpp"

#include "../../base/matrix.hpp"
#include "../../base/exception.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

namespace graphics
{
  class VertexDecl;
  namespace gl
  {
    class Shader;

    class Program
    {
    private:

      GLuint m_handle;

      typedef map<string, function<void()> > TUniforms;
      TUniforms m_uniforms;

      typedef map<GLuint, function<void()> > TAttributes;
      TAttributes m_attributes;

      Storage m_storage;

    public:

      DECLARE_EXCEPTION(Exception, RootException);
      DECLARE_EXCEPTION(LinkException, Exception);

      Program(shared_ptr<Shader> const & vxShader,
              shared_ptr<Shader> const & frgShader);

      ~Program();

      unsigned getParam(char const * name);

      void setParam(char const * name, float v0);
      void setParam(char const * name, float v0, float v1);
      void setParam(char const * name, float v0, float v1, float v2);
      void setParam(char const * name, float v0, float v1, float v2, float v3);

      void setParam(char const * name, int v0);
      void setParam(char const * name, int v0, int v1);
      void setParam(char const * name, int v0, int v1, int v2);
      void setParam(char const * name, int v0, int v1, int v2, int v3);

      void setParam(char const * name, math::Matrix<float, 2, 2> const & m);
      void setParam(char const * name, math::Matrix<float, 3, 3> const & m);
      void setParam(char const * name, math::Matrix<float, 4, 4> const & m);

      GLuint getAttribute(char const * name);
      void setAttribute(GLuint id, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void * ptr);

      void setVertexDecl(VertexDecl const * decl);

      void setStorage(Storage const & storage);

      void makeCurrent();
    };
  }
}
