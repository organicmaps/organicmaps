#pragma once

#include "opengl.hpp"
#include "storage.hpp"
#include "buffer_object.hpp"

#include "../defines.hpp"

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

      void applyAttributes();
      void applyUniforms();

      struct Uniform
      {
        struct Data
        {
          float m_matVal[4 * 4];
          int m_intVal[4];
          float m_floatVal[4];
        } m_data;
        EDataType m_type;
        GLint m_handle;
        bool m_changed = true;
      };

      struct Attribute
      {
        GLint m_handle;
        size_t m_offset;
        EDataType m_type;
        size_t m_count;
        size_t m_stride;
      };

      bool m_changed = true;
      GLuint m_handle;

      typedef map<ESemantic, Uniform> TUniforms;
      TUniforms m_uniforms;

      typedef map<ESemantic, Attribute> TAttributes;
      TAttributes m_attributes;

      Storage m_storage;

      template <unsigned N>
      void setParamImpl(ESemantic sem,
                        EDataType dt,
                        math::Matrix<float, N, N> const & m);

    public:
      DECLARE_EXCEPTION(Exception, RootException);
      DECLARE_EXCEPTION(LinkException, Exception);

      Program(shared_ptr<Shader> const & vxShader,
              shared_ptr<Shader> const & frgShader);

      ~Program();

      bool isParamExist(ESemantic sem) const;

      void setParam(ESemantic sem, float v0);
      void setParam(ESemantic sem, float v0, float v1);
      void setParam(ESemantic sem, float v0, float v1, float v2);
      void setParam(ESemantic sem, float v0, float v1, float v2, float v3);

      void setParam(ESemantic sem, int v0);
      void setParam(ESemantic sem, int v0, int v1);
      void setParam(ESemantic sem, int v0, int v1, int v2);
      void setParam(ESemantic sem, int v0, int v1, int v2, int v3);

      void setParam(ESemantic sem, math::Matrix<float, 2, 2> const & m);
      void setParam(ESemantic sem, math::Matrix<float, 3, 3> const & m);
      void setParam(ESemantic sem, math::Matrix<float, 4, 4> const & m);

      void setVertexDecl(VertexDecl const * decl);

      void setStorage(Storage const & storage);

      void makeCurrent(shared_ptr<gl::BufferObject::Binder> & bindVerticesBuf, shared_ptr<gl::BufferObject::Binder> & bindIndicesBuf);
      void riseChangedFlag();
    };
  }
}
