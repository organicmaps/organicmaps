#include "program.hpp"
#include "shader.hpp"
#include "buffer_object.hpp"
#include "defines_conv.hpp"
#include "../../base/thread.hpp"

#include "../../std/bind.hpp"
#include "../vertex_decl.hpp"

namespace graphics
{
  namespace gl
  {
    Program::Program(shared_ptr<Shader> const & vxShader,
                     shared_ptr<Shader> const & frgShader)
    {
      m_handle = glCreateProgramFn();
      OGLCHECKAFTER;

      if (!m_handle)
        throw Exception("CreateProgram error", "could not create Program!");

      OGLCHECK(glAttachShaderFn(m_handle, vxShader->id()));
      OGLCHECK(glAttachShaderFn(m_handle, frgShader->id()));

      OGLCHECK(glLinkProgramFn(m_handle));

      int linkStatus = GL_FALSE;
      OGLCHECK(glGetProgramivFn(m_handle, GL_LINK_STATUS, &linkStatus));

      if (linkStatus != GL_TRUE)
      {
        int bufLength = 0;
        OGLCHECK(glGetProgramivFn(m_handle, GL_INFO_LOG_LENGTH, &bufLength));
        if (bufLength)
        {
          vector<char> v;
          v.resize(bufLength);
          glGetProgramInfoLogFn(m_handle, bufLength, NULL, &v[0]);

          throw LinkException("Could not link program: ", &v[0]);
        }

        throw LinkException("Could not link program: ", "Unknown link error");
      }
    }

    Program::~Program()
    {
      OGLCHECK(glDeleteProgramFn(m_handle));
    }

    GLuint Program::getParam(char const * name)
    {
      GLuint res = glGetUniformLocationFn(m_handle, name);
      OGLCHECKAFTER;
      return res;
    }

    GLuint Program::getAttribute(char const * name)
    {
      GLuint res = glGetAttribLocationFn(m_handle, name);
      OGLCHECKAFTER;
      return res;
    }

    void setParamImpl(GLuint prgID, char const * name, function<void(GLint)> fn)
    {
      GLuint res = glGetUniformLocationFn(prgID, name);
      OGLCHECKAFTER;
      OGLCHECK(fn(res));
    }

    void Program::setParam(char const * name, float v0)
    {
      function<void(GLint)> fn = bind(&glUniform1f, _1, v0);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, float v0, float v1)
    {
      function<void(GLint)> fn = bind(&glUniform2f, _1, v0, v1);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, float v0, float v1, float v2)
    {
      function<void(GLint)> fn = bind(&glUniform3f, _1, v0, v1, v2);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, float v0, float v1, float v2, float v3)
    {
      function<void(GLint)> fn = bind(&glUniform4f, _1, v0, v1, v2, v3);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, int v0)
    {
      function<void(GLint)> fn = bind(&glUniform1i, _1, v0);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, int v0, int v1)
    {
      function<void(GLint)> fn = bind(&glUniform2i, _1, v0, v1);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, int v0, int v1, int v2)
    {
      function<void(GLint)> fn = bind(&glUniform3i, _1, v0, v1, v2);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, int v0, int v1, int v2, int v3)
    {
      function<void(GLint)> fn = bind(&glUniform4i, _1, v0, v1, v2, v3);
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, math::Matrix<float, 2, 2> const & m)
    {
      function<void(GLint)> fn = bind(&glUniformMatrix2fv, _1, 1, 0, &m(0, 0));
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, math::Matrix<float, 3, 3> const & m)
    {
      function<void(GLint)> fn = bind(&glUniformMatrix3fv, _1, 1, 0, &m(0, 0));
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void Program::setParam(char const * name, math::Matrix<float, 4, 4> const & m)
    {
      function<void(GLint)> fn = bind(&glUniformMatrix4fv, _1, 1, 0, &m(0, 0));
      m_uniforms[name] = bind(&setParamImpl, m_handle, name, fn);
    }

    void enableAndSetVertexAttrib(GLuint id, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void * ptr)
    {
      OGLCHECK(glEnableVertexAttribArray(id));
      OGLCHECK(glVertexAttribPointer(id, size, type, normalized, stride, ptr));
    }

    void Program::setAttribute(GLuint id, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void *ptr)
    {
      function<void()> fn =  bind(&enableAndSetVertexAttrib, id, size, type, normalized, stride, ptr);
      m_attributes[id] = fn;
    }

    void Program::setVertexDecl(VertexDecl const * decl)
    {
      for (size_t i = 0; i < decl->size(); ++i)
      {
        VertexAttrib const * va = decl->getAttr(i);
        GLuint attrID = getAttribute(va->m_name.c_str());
        GLenum glType;
        convert(va->m_elemType, glType);
        setAttribute(attrID,
                     va->m_elemCount,
                     glType,
                     false,
                     va->m_stride,
                     (void*)((unsigned char*)m_storage.m_vertices->glPtr() + va->m_offset));
      }
    }

    void Program::setStorage(Storage const & storage)
    {
      m_storage = storage;
    }

    void Program::makeCurrent()
    {
      OGLCHECK(glUseProgramFn(m_handle));

      m_storage.m_vertices->makeCurrent();

      /// setting all attributes streams;
      for (TAttributes::const_iterator it = m_attributes.begin();
           it != m_attributes.end();
           ++it)
      {
        it->second();
      }

      m_storage.m_indices->makeCurrent();

      /// setting all uniforms
      for (TUniforms::const_iterator it = m_uniforms.begin();
           it != m_uniforms.end();
           ++it)
        it->second();
    }
  }
}


