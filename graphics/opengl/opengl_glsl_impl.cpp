#include "graphics/opengl/opengl_glsl_impl.hpp"

#include "graphics/opengl/opengl.hpp"

#include "base/matrix.hpp"
#include "base/thread.hpp"

#include "std/memcpy.hpp"

namespace graphics
{
  namespace gl
  {
    const GLenum GL_MODELVIEW_MWM = 0;
    const GLenum GL_PROJECTION_MWM = 1;

    const GLenum GL_VERTEX_ARRAY_MWM = 0;
    const GLenum GL_TEXTURE_COORD_ARRAY_MWM = 1;
    const GLenum GL_NORMAL_ARRAY_MWM = 2;

    const GLenum GL_ALPHA_TEST_MWM = 0x0BC0;

#if defined(OMIM_GL_ES)
  #define PRECISION "lowp"
#else
  #define PRECISION ""
#endif

    namespace glsl
    {
      /// Vertex Shader Source

      static const char g_vxSrc[] =
        "attribute vec4 Position;\n"
        "attribute vec2 Normal;\n"
        "attribute vec2 TexCoordIn;\n"
        "uniform mat4 ProjM;\n"
        "uniform mat4 ModelViewM;\n"
        "varying vec2 TexCoordOut;\n"
        "void main(void) {\n"
        "   gl_Position = (vec4(Normal, 0.0, 0.0) + Position * ModelViewM) * ProjM;\n"
        "   TexCoordOut = TexCoordIn;\n"
        "}\n";

      /// Sharp Vertex Shader Source

      static const char g_sharpVxSrc[] =
        "attribute vec4 Position;\n"
        "attribute vec2 Normal;\n"
        "attribute vec2 TexCoordIn;\n"
        "uniform mat4 ProjM;\n"
        "uniform mat4 ModelViewM;\n"
        "varying vec2 TexCoordOut;\n"
        "void main(void) {\n"
        "   gl_Position = floor(vec4(Normal, 0.0, 0.0) + Position * ModelViewM) * ProjM;\n"
        "   TexCoordOut = TexCoordIn;\n"
        "}\n";

      /// Fragment Shader with alphaTest

      static const char g_alphaTestFrgSrc [] =
        "uniform sampler2D Texture;\n"
        "varying " PRECISION " vec2 TexCoordOut;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Texture, TexCoordOut);\n"
        "   if (gl_FragColor.a == 0.0)\n"
        "     discard;\n"
        "}\n";

      /// Fragment shader without alphaTest

      static const char g_noAlphaTestFrgSrc[] =
        "uniform sampler2D Texture;\n"
        "varying " PRECISION " vec2 TexCoordOut;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Texture, TexCoordOut);\n"
        "}\n";

      /// Structure that holds a single GLSL program
      /// along with handles to attributes and uniforms
      struct Program
      {
        GLuint m_program;
        GLuint m_positionHandle;
        GLuint m_texCoordHandle;
        GLuint m_normalHandle;
        GLuint m_projectionUniform;
        GLuint m_modelViewUniform;
        GLuint m_textureUniform;

        bool createProgram(GLuint vertexShader, GLuint fragmentShader)
        {
          m_program = ::glCreateProgram();
          OGLCHECKAFTER;

          if (!m_program)
            return false;

          OGLCHECK(::glAttachShader(m_program, vertexShader));
          OGLCHECK(::glAttachShader(m_program, fragmentShader));
          OGLCHECK(::glLinkProgram(m_program));

          int linkStatus = GL_FALSE;
          OGLCHECK(::glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus));

          if (linkStatus != GL_TRUE)
          {
            int bufLength = 0;
            OGLCHECK(::glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &bufLength));
            if (bufLength)
            {
              char * buf = new char[bufLength];
              ::glGetProgramInfoLog(m_program, bufLength, NULL, buf);
              LOG(LINFO, ("Could not link program:"));
              LOG(LINFO, (buf));
              delete [] buf;
            }

            return false;
          }

          return true;
        }

        void attachProjection(char const * name)
        {
          m_projectionUniform = ::glGetUniformLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void attachModelView(char const * name)
        {
          m_modelViewUniform = ::glGetUniformLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void attachTexture(char const * name)
        {
          m_textureUniform = ::glGetUniformLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void attachNormal(char const * name)
        {
          m_normalHandle = ::glGetAttribLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void attachPosition(char const * name)
        {
          m_positionHandle = ::glGetAttribLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void attachTexCoord(char const * name)
        {
          m_texCoordHandle = ::glGetAttribLocation(m_program, name);
          OGLCHECKAFTER;
        }

        void apply()
        {
          OGLCHECK(::glUseProgram(m_program));
        }
      };

      struct ThreadData
      {
        GLenum m_matrixMode;
        math::Matrix<float, 4, 4> m_matrices[2];

        Program m_noAlphaTestProgram;
        Program m_alphaTestProgram;

        Program m_noAlphaTestSharpProgram;
        Program m_alphaTestSharpProgram;

        /// currently bound GLSL program
        Program * m_currentProgram;

        GLuint m_vxShader;
        GLuint m_sharpVxShader;
        GLuint m_noAlphaTestFrgSh;
        GLuint m_alphaTestFrgSh;

        GLboolean m_useAlphaTest;
        GLboolean m_useSharpGeometry;

        Program * selectCurrentProgram()
        {
          if (m_useAlphaTest)
            if (m_useSharpGeometry)
              return &m_alphaTestSharpProgram;
            else
              return &m_alphaTestProgram;
          else
            if (m_useSharpGeometry)
              return &m_noAlphaTestSharpProgram;
            else
              return &m_noAlphaTestProgram;
        }

        void setCurrentProgram(Program * program)
        {
          if (m_currentProgram != program)
          {
            m_currentProgram = program;
            m_currentProgram->apply();
          }
        }

        GLuint loadShader(char const * shaderSource, GLenum shaderType)
        {
          GLuint res = ::glCreateShader(shaderType);
          OGLCHECKAFTER;

          int len = strlen(shaderSource);

          OGLCHECK(::glShaderSource(res, 1, &shaderSource, &len));

          OGLCHECK(::glCompileShader(res));

          GLint compileRes;
          OGLCHECK(::glGetShaderiv(res, GL_COMPILE_STATUS, &compileRes));

          if (compileRes == GL_FALSE)
          {
            GLchar msg[256];
            OGLCHECK(::glGetShaderInfoLog(res, sizeof(msg), 0, msg));
            LOG(LINFO, ("Couldn't compile shader :"));
            LOG(LERROR, (msg));
            return 0;
          }

          return res;
        }

        ThreadData()
          : m_matrixMode(-1),
            m_vxShader(0),
            m_sharpVxShader(0),
            m_noAlphaTestFrgSh(0),
            m_alphaTestFrgSh(0),
            m_useAlphaTest(false),
            m_useSharpGeometry(false)
        {}

        void Initialize()
        {
          m_vxShader = loadShader(g_vxSrc, GL_VERTEX_SHADER);
          m_sharpVxShader = loadShader(g_sharpVxSrc, GL_VERTEX_SHADER);

          m_noAlphaTestFrgSh = loadShader(g_noAlphaTestFrgSrc, GL_FRAGMENT_SHADER);
          m_alphaTestFrgSh = loadShader(g_alphaTestFrgSrc, GL_FRAGMENT_SHADER);

          /// creating program
          m_alphaTestProgram.createProgram(m_vxShader, m_alphaTestFrgSh);
          m_noAlphaTestProgram.createProgram(m_vxShader, m_noAlphaTestFrgSh);

          m_alphaTestSharpProgram.createProgram(m_sharpVxShader, m_alphaTestFrgSh);
          m_noAlphaTestSharpProgram.createProgram(m_sharpVxShader, m_noAlphaTestFrgSh);

          m_alphaTestProgram.attachProjection("ProjM");
          m_alphaTestProgram.attachModelView("ModelViewM");
          m_alphaTestProgram.attachTexture("Texture");
          m_alphaTestProgram.attachPosition("Position");
          m_alphaTestProgram.attachTexCoord("TexCoordIn");
          m_alphaTestProgram.attachNormal("Normal");

          m_alphaTestSharpProgram.attachProjection("ProjM");
          m_alphaTestSharpProgram.attachModelView("ModelViewM");
          m_alphaTestSharpProgram.attachTexture("Texture");
          m_alphaTestSharpProgram.attachPosition("Position");
          m_alphaTestSharpProgram.attachTexCoord("TexCoordIn");
          m_alphaTestSharpProgram.attachNormal("Normal");

          m_noAlphaTestProgram.attachProjection("ProjM");
          m_noAlphaTestProgram.attachModelView("ModelViewM");
          m_noAlphaTestProgram.attachTexture("Texture");
          m_noAlphaTestProgram.attachPosition("Position");
          m_noAlphaTestProgram.attachTexCoord("TexCoordIn");
          m_noAlphaTestProgram.attachNormal("Normal");

          m_noAlphaTestSharpProgram.attachProjection("ProjM");
          m_noAlphaTestSharpProgram.attachModelView("ModelViewM");
          m_noAlphaTestSharpProgram.attachTexture("Texture");
          m_noAlphaTestSharpProgram.attachPosition("Position");
          m_noAlphaTestSharpProgram.attachTexCoord("TexCoordIn");
          m_noAlphaTestSharpProgram.attachNormal("Normal");

          selectCurrentProgram()->apply();
        }

        void Finalize()
        {
          if (graphics::gl::g_hasContext)
            ::glDeleteProgram(m_alphaTestProgram.m_program);
          if (graphics::gl::g_hasContext)
            ::glDeleteProgram(m_noAlphaTestProgram.m_program);
          if (graphics::gl::g_hasContext)
            ::glDeleteProgram(m_alphaTestSharpProgram.m_program);
          if (graphics::gl::g_hasContext)
            ::glDeleteProgram(m_noAlphaTestSharpProgram.m_program);
          if (graphics::gl::g_hasContext)
            ::glDeleteShader(m_vxShader);
          if (graphics::gl::g_hasContext)
            ::glDeleteShader(m_sharpVxShader);
          if (graphics::gl::g_hasContext)
            ::glDeleteShader(m_noAlphaTestFrgSh);
          if (graphics::gl::g_hasContext)
            ::glDeleteShader(m_alphaTestFrgSh);
        }
      };

      typedef map<threads::ThreadID, glsl::ThreadData> ThreadDataMap;
      ThreadDataMap g_threadData;

      void glEnable(GLenum cap)
      {
        if (cap == GL_ALPHA_TEST_MWM)
        {
          ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
          threadData.m_useAlphaTest = true;
        }
        else
          ::glEnable(cap);
      }

      void glDisable(GLenum cap)
      {
        if (cap == GL_ALPHA_TEST_MWM)
        {
          ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
          threadData.m_useAlphaTest = false;
        }
        else
          ::glDisable(cap);
      }

      void glAlphaFunc(GLenum func, GLclampf ref)
      {
        ASSERT((func == GL_NOTEQUAL) && (ref == 0.0), ("only GL_NOEQUAL with 0.0 reference value is supported for alphaTest"));
      }

      void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        ::glVertexAttribPointer(threadData.m_alphaTestProgram.m_positionHandle, size, type, GL_FALSE, stride, pointer);
        ::glVertexAttribPointer(threadData.m_noAlphaTestProgram.m_positionHandle, size, type, GL_FALSE, stride, pointer);
      }

      void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        ::glVertexAttribPointer(threadData.m_alphaTestProgram.m_texCoordHandle, size, type, GL_FALSE, stride, pointer);
        ::glVertexAttribPointer(threadData.m_noAlphaTestProgram.m_texCoordHandle, size, type, GL_FALSE, stride, pointer);
      }

      void glNormalPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        ::glVertexAttribPointer(threadData.m_alphaTestProgram.m_normalHandle, size, type, GL_FALSE, stride, pointer);
        ::glVertexAttribPointer(threadData.m_noAlphaTestProgram.m_normalHandle, size, type, GL_FALSE, stride, pointer);
      }

      void glEnableClientState(GLenum array)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        switch (array)
        {
        case GL_VERTEX_ARRAY_MWM:
          ::glEnableVertexAttribArray(threadData.m_alphaTestProgram.m_positionHandle);
          ::glEnableVertexAttribArray(threadData.m_noAlphaTestProgram.m_positionHandle);
          break;
        case GL_TEXTURE_COORD_ARRAY_MWM:
          ::glEnableVertexAttribArray(threadData.m_alphaTestProgram.m_texCoordHandle);
          ::glEnableVertexAttribArray(threadData.m_noAlphaTestProgram.m_texCoordHandle);
          break;
        case GL_NORMAL_ARRAY_MWM:
          ::glEnableVertexAttribArray(threadData.m_alphaTestProgram.m_normalHandle);
          ::glEnableVertexAttribArray(threadData.m_noAlphaTestProgram.m_normalHandle);
          break;
        default:
          LOG(LERROR, ("Unknown option is passed to glEnableClientState"));
        };
      }

      void glUseSharpGeometry(GLboolean flag)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        threadData.m_useSharpGeometry = flag;
      }

      void glMatrixMode(GLenum mode)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        threadData.m_matrixMode = mode;
      }

      void glLoadIdentity()
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];
        threadData.m_matrices[threadData.m_matrixMode] = math::Identity<float, 4>();
      }

      void glLoadMatrixf(GLfloat const * d)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];

        math::Matrix<float, 4, 4> m;

        m(0, 0) = d[0]; m(0, 1) = d[1]; m(0, 2) = d[2]; m(0, 3) = d[3];
        m(1, 0) = d[4]; m(1, 1) = d[5]; m(1, 2) = d[6]; m(1, 3) = d[7];
        m(2, 0) = d[8]; m(2, 1) = d[9]; m(2, 2) = d[10]; m(2, 3) = d[11];
        m(3, 0) = d[12]; m(3, 1) = d[13]; m(3, 2) = d[14]; m(3, 3) = d[15];

        threadData.m_matrices[threadData.m_matrixMode] = m * threadData.m_matrices[threadData.m_matrixMode];
      }

      void glOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];

        math::Matrix<float, 4, 4> m = math::Identity<float, 4>();

        m(0, 0) = 2 / (r - l); m(0, 1) = 0;          m(0, 2) = 0;            m(0, 3) = -(r + l) / (r - l);
        m(1, 0) = 0;           m(1, 1) = 2 / (t - b);m(1, 2) = 0;            m(1, 3) = -(t + b) / (t - b);
        m(2, 0) = 0;           m(2, 1) = 0;          m(2, 2) = -2 / (f - n); m(2, 3) = - (f + n) / (f - n);
        m(3, 0) = 0;           m(3, 1) = 0;          m(3, 2) = 0;            m(3, 3) = 1;

        threadData.m_matrices[threadData.m_matrixMode] = m * threadData.m_matrices[threadData.m_matrixMode];
      }

      void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
      {
        ThreadData & threadData = g_threadData[threads::GetCurrentThreadID()];

        math::Matrix<float, 4, 4> const & projM = threadData.m_matrices[GL_PROJECTION_MWM];
        math::Matrix<float, 4, 4> const & modelViewM = threadData.m_matrices[GL_MODELVIEW_MWM];

        threadData.setCurrentProgram(threadData.selectCurrentProgram());

        // applying shader parameters
        OGLCHECK(::glUniformMatrix4fv(threadData.m_currentProgram->m_projectionUniform, 1, 0, &projM(0, 0)));
        OGLCHECK(::glUniformMatrix4fv(threadData.m_currentProgram->m_modelViewUniform, 1, 0, &modelViewM(0, 0)));
        OGLCHECK(::glUniform1i(threadData.m_currentProgram->m_textureUniform, 0));

        // drawing elements
        OGLCHECK(::glDrawElements(mode, count, type, indices));
      }
    }

    void InitializeThread()
    {
      threads::ThreadID id = threads::GetCurrentThreadID();
      glsl::ThreadDataMap::const_iterator it = glsl::g_threadData.find(id);

      if (it != glsl::g_threadData.end())
      {
        LOG(LWARNING, ("GLSL structures for thread", threads::GetCurrentThreadID(), "is already initialized"));
        return;
      }

      LOG(LINFO, ("initializing GLSL structures for thread", threads::GetCurrentThreadID()));
      glsl::g_threadData[id].Initialize();
    }

    void FinalizeThread()
    {
      threads::ThreadID id = threads::GetCurrentThreadID();
      glsl::ThreadDataMap::const_iterator it = glsl::g_threadData.find(id);

      if (it == glsl::g_threadData.end())
      {
        LOG(LWARNING, ("GLSL structures for thread", threads::GetCurrentThreadID(), "is already finalized"));
        return;
      }

      LOG(LINFO, ("finalizing GLSL structures for thread", threads::GetCurrentThreadID()));

      glsl::g_threadData[id].Finalize();
      glsl::g_threadData.erase(id);
    }
  }
}
