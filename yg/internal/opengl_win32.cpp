#include "opengl.hpp"

#include "../../3party/GL/glext.h"

#include "../../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    GL_FRAMEBUFFER_BINDING = GL_FRAMEBUFFER_BINDING_EXT;

    template <class TRet>
    TRet GetGLProc(HMODULE, char const * name)
    {
      PROC p = ::wglGetProcAddress(name);
      if (p == 0)
      {
        DWORD const err = ::GetLastError();
        LOG(LINFO, ("OpenGL extension function ", name, " not found. Last error = ", err));
      }
      return reinterpret_cast<TRet>(p);
    }

    void InitExtensions()
    {
      HMODULE hInst = 0;

      // Loading procedures, trying "EXT" suffix if alias doesn't exist

#define DEFINE_GL_PROC_WIN32(type, name, fn) \
      fn = GetGLProc<type>(hInst, name);             \
      if (fn == NULL)
      {
        string extName = string(name) + "EXT";
        fn = GetGLProc<type>(hInst, extName);
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

#endif
