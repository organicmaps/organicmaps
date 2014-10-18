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

      /// getting all uniforms
      int cnt = 0;

      OGLCHECK(glGetProgramivFn(m_handle, GL_ACTIVE_UNIFORMS, &cnt));

      GLchar name[1024];
      GLsizei len = 0;
      GLint size;
      GLenum type;

      for (unsigned i = 0; i < cnt; ++i)
      {
        Uniform f;
        ESemantic sem;

        OGLCHECK(glGetActiveUniformFn(m_handle, i, ARRAY_SIZE(name), &len, &size, &type, name));

        f.m_handle = glGetUniformLocationFn(m_handle, name);
        OGLCHECKAFTER;

        convert(type, f.m_type);
        convert(name, sem);

        m_uniforms[sem] = f;
      }

      OGLCHECK(glGetProgramivFn(m_handle, GL_ACTIVE_ATTRIBUTES, &cnt));

      for (unsigned i = 0; i < cnt; ++i)
      {
        Attribute a;
        ESemantic sem;

        OGLCHECK(glGetActiveAttribFn(m_handle, i, ARRAY_SIZE(name), &len, &size, &type, name));

        a.m_handle = glGetAttribLocationFn(m_handle, name);
        OGLCHECKAFTER;

        convert(type, a.m_type, a.m_count);
        convert(name, sem);

        m_attributes[sem] = a;
      }
    }

    Program::~Program()
    {
      OGLCHECK(glDeleteProgramFn(m_handle));
    }

    bool Program::isParamExist(ESemantic sem) const
    {
      return m_uniforms.find(sem) != m_uniforms.end();
    }

    void Program::setParam(ESemantic sem, float v0)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EFloat;
      if (f.m_data.m_floatVal[0] != v0)
      {
        f.m_data.m_floatVal[0] = v0;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, float v0, float v1)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EFloatVec2;
      if (f.m_data.m_floatVal[0] != v0 ||
          f.m_data.m_floatVal[1] != v1)
      {
        f.m_data.m_floatVal[0] = v0;
        f.m_data.m_floatVal[1] = v1;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, float v0, float v1, float v2)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EFloatVec3;
      if (f.m_data.m_floatVal[0] != v0 ||
          f.m_data.m_floatVal[1] != v1 ||
          f.m_data.m_floatVal[2] != v2)
      {
        f.m_data.m_floatVal[0] = v0;
        f.m_data.m_floatVal[1] = v1;
        f.m_data.m_floatVal[2] = v2;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, float v0, float v1, float v2, float v3)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EFloatVec4;
      if (f.m_data.m_floatVal[0] != v0 ||
          f.m_data.m_floatVal[1] != v1 ||
          f.m_data.m_floatVal[2] != v2 ||
          f.m_data.m_floatVal[3] != v3)
      {
        f.m_data.m_floatVal[0] = v0;
        f.m_data.m_floatVal[1] = v1;
        f.m_data.m_floatVal[2] = v2;
        f.m_data.m_floatVal[3] = v3;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, int v0)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EInteger;
      if (f.m_data.m_intVal[0] != v0)
      {
        f.m_data.m_intVal[0] = v0;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, int v0, int v1)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EIntegerVec2;
      if (f.m_data.m_intVal[0] != v0 ||
          f.m_data.m_intVal[1] != v1)
      {
        f.m_data.m_intVal[0] = v0;
        f.m_data.m_intVal[1] = v1;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, int v0, int v1, int v2)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EIntegerVec3;
      if (f.m_data.m_intVal[0] != v0 ||
          f.m_data.m_intVal[1] != v1 ||
          f.m_data.m_intVal[2] != v2)
      {
        f.m_data.m_intVal[0] = v0;
        f.m_data.m_intVal[1] = v1;
        f.m_data.m_intVal[2] = v2;
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, int v0, int v1, int v2, int v3)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = EIntegerVec4;
      if (f.m_data.m_intVal[0] != v0 ||
          f.m_data.m_intVal[1] != v1 ||
          f.m_data.m_intVal[2] != v2 ||
          f.m_data.m_intVal[3] != v3)
      {
        f.m_data.m_intVal[0] = v0;
        f.m_data.m_intVal[1] = v1;
        f.m_data.m_intVal[2] = v2;
        f.m_data.m_intVal[3] = v3;
        f.m_changed = true;
      }
    }

    template <unsigned N>
    void Program::setParamImpl(ESemantic sem,
                               EDataType dt,
                               math::Matrix<float, N, N> const & m)
    {
      map<ESemantic, Uniform>::iterator it = m_uniforms.find(sem);
      ASSERT(it != m_uniforms.end(), ());
      Uniform & f = it->second;

      f.m_type = dt;
      if (!equal(&m(0, 0), &m(0, 0) + N * N, f.m_data.m_matVal))
      {
        copy(&m(0, 0), &m(0, 0) + N * N, f.m_data.m_matVal);
        f.m_changed = true;
      }
    }

    void Program::setParam(ESemantic sem, math::Matrix<float, 2, 2> const & m)
    {
      setParamImpl(sem, EFloatMat2, m);
    }

    void Program::setParam(ESemantic sem, math::Matrix<float, 3, 3> const & m)
    {
      setParamImpl(sem, EFloatMat3, m);
    }

    void Program::setParam(ESemantic sem, math::Matrix<float, 4, 4> const & m)
    {
      setParamImpl(sem, EFloatMat4, m);
    }

    void Program::setVertexDecl(VertexDecl const * decl)
    {
      for (size_t i = 0; i < decl->attrCount(); ++i)
      {
        VertexAttrib const * va = decl->getAttr(i);

        map<ESemantic, Attribute>::iterator it = m_attributes.find(va->m_semantic);
        ASSERT(it != m_attributes.end(), ());
        Attribute & a = it->second;

        a.m_offset = va->m_offset;
        a.m_stride = va->m_stride;
        a.m_count = va->m_elemCount;

        /// a.m_count could be different from va->m_elemCount
        /// as GLSL could provide missing attribute components
        /// with default values according to internal rules.
        /// (f.e. all components except 4th in vec4 are made 0
        /// by default, and 4th is 1 by default).

//        ASSERT(a.m_count == va->m_elemCount, ());
        ASSERT(a.m_type == va->m_elemType, ());
      }
    }

    void Program::setStorage(Storage const & storage)
    {
      m_storage = storage;
    }

    void Program::applyAttributes()
    {
      /// setting all attributes streams;
      for (TAttributes::const_iterator it = m_attributes.begin();
           it != m_attributes.end();
           ++it)
      {
        Attribute const & a = it->second;

        GLenum t;
        convert(a.m_type, t);

        OGLCHECK(glEnableVertexAttribArrayFn(a.m_handle));
        OGLCHECK(glVertexAttribPointerFn(a.m_handle,
                                         a.m_count,
                                         t,
                                         false,
                                         a.m_stride,
                                         (void*)((unsigned char *)m_storage.m_vertices->glPtr() + a.m_offset)));
      }
    }

    void Program::applyUniforms()
    {
      /// setting all uniforms
      typedef pair<ESemantic const, Uniform> TNode;
      for (TNode & node : m_uniforms)
      {
        Uniform & u = node.second;
        if (!u.m_changed)
          continue;

        u.m_changed = false;

        switch (u.m_type)
        {
        case EFloat:
          OGLCHECK(glUniform1fFn(u.m_handle,
                                 u.m_data.m_floatVal[0]));
          break;
        case EFloatVec2:
          OGLCHECK(glUniform2fFn(u.m_handle,
                                 u.m_data.m_floatVal[0],
                                 u.m_data.m_floatVal[1]));
          break;
        case EFloatVec3:
          OGLCHECK(glUniform3fFn(u.m_handle,
                                 u.m_data.m_floatVal[0],
                                 u.m_data.m_floatVal[1],
                                 u.m_data.m_floatVal[2]));
          break;
        case EFloatVec4:
          OGLCHECK(glUniform4fFn(u.m_handle,
                                 u.m_data.m_floatVal[0],
                                 u.m_data.m_floatVal[1],
                                 u.m_data.m_floatVal[2],
                                 u.m_data.m_floatVal[3]));
          break;
        case EInteger:
          OGLCHECK(glUniform1iFn(u.m_handle,
                                 u.m_data.m_intVal[0]));
          break;
        case EIntegerVec2:
          OGLCHECK(glUniform2iFn(u.m_handle,
                                 u.m_data.m_intVal[0],
                                 u.m_data.m_intVal[1]));
          break;
        case EIntegerVec3:
          OGLCHECK(glUniform3iFn(u.m_handle,
                                 u.m_data.m_intVal[0],
                                 u.m_data.m_intVal[1],
                                 u.m_data.m_intVal[2]));
          break;
        case EIntegerVec4:
          OGLCHECK(glUniform4iFn(u.m_handle,
                                 u.m_data.m_intVal[0],
                                 u.m_data.m_intVal[1],
                                 u.m_data.m_intVal[2],
                                 u.m_data.m_intVal[3]));
          break;
        case EFloatMat2:
          OGLCHECK(glUniformMatrix2fvFn(u.m_handle,
                                        1,
                                        false,
                                        u.m_data.m_matVal));
          break;
        case EFloatMat3:
          OGLCHECK(glUniformMatrix3fvFn(u.m_handle,
                                        1,
                                        false,
                                        u.m_data.m_matVal));
          break;
        case EFloatMat4:
          OGLCHECK(glUniformMatrix4fvFn(u.m_handle,
                                        1,
                                        false,
                                        u.m_data.m_matVal));
          break;
        case ESampler2D:
          OGLCHECK(glUniform1iFn(u.m_handle,
                               u.m_data.m_intVal[0]));
          break;
        }
      }
    }

    void Program::makeCurrent()
    {
      if (m_changed)
      {

        OGLCHECK(glUseProgramFn(m_handle));
        m_changed = false;
      }

      m_storage.m_vertices->makeCurrent();

      applyAttributes();

      m_storage.m_indices->makeCurrent();

      applyUniforms();
    }

    void Program::riseChangedFlag()
    {
      m_changed = true;
    }
  }
}


