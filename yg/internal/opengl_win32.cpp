#include "opengl.hpp"

//#include "../../3party/GL/glext.h"

#include "../../base/logging.hpp"


namespace yg
{
  namespace gl
  {
    //GL_FRAMEBUFFER_BINDING = GL_FRAMEBUFFER_BINDING_EXT;

    template <class T>
    void AssignGLProc(HMODULE, char const * name, T & res)
    {
      PROC p = ::wglGetProcAddress(name);
      if (p == 0)
      {
        DWORD const err = ::GetLastError();
        LOG(LINFO, ("OpenGL extension function ", name, " not found. Last error = ", err));
      }
      res = reinterpret_cast<T>(p);
    }

    void InitExtensions()
    {
      HMODULE hInst = 0;

      // Loading procedures, trying "EXT" suffix if alias doesn't exist

#define DEFINE_GL_PROC(name, fn)                      \
      AssignGLProc(hInst, name, fn);                  \
      if (fn == NULL)                                 \
      {                                               \
        string const extName = string(name) + "EXT";  \
        AssignGLProc(hInst, extName.c_str(), fn);     \
      }
#include "gl_procedures.inl"
#undef DEFINE_GL_PROC

      yg::gl::g_isBufferObjectsSupported = glBindBufferFn
                                        && glGenBuffersFn
                                        && glBufferDataFn
                                        && glBufferSubDataFn
                                        && glDeleteBuffersFn
                                        && glMapBufferFn
                                        && glUnmapBufferFn;

      yg::gl::g_isFramebufferSupported = glBindFramebufferFn
                                      && glFramebufferTexture2DFn
                                      && glFramebufferRenderbufferFn
                                      && glGenFramebuffersFn
                                      && glDeleteFramebuffersFn
                                      && glCheckFramebufferStatusFn;

      yg::gl::g_isRenderbufferSupported = glGenRenderbuffersFn
                                       && glDeleteRenderbuffersFn
                                       && glBindRenderbufferFn
                                       && glRenderbufferStorageFn;
    }
  }
} // namespace win32
