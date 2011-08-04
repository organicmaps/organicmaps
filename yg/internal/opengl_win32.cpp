#include "../../base/SRC_FIRST.hpp"

#include "opengl_win32.hpp"

#include "../../base/logging.hpp"

#include "../../base/start_mem_debug.hpp"

#ifdef OMIM_OS_WINDOWS

#define DEFINE_GL_PROC(type, name) \
  type name;
#include "gl_procedures.inl"
#undef DEFINE_GL_PROC

namespace win32
{
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

  void InitOpenGL()
  {
    HMODULE hInst = 0;

    // Loading procedures, trying "EXT" suffix if alias doesn't exist
#define DEFINE_GL_PROC(type, name) \
    name = GetGLProc<type>(hInst, #name);             \
    if (name == NULL)                                 \
      name = GetGLProc<type>(hInst, #name ## "EXT");
#include "gl_procedures.inl"
#undef DEFINE_GL_PROC

    yg::gl::g_isBufferObjectsSupported = glBindBuffer
                              && glGenBuffers
                              && glBufferData
                              && glBufferSubData
                              && glDeleteBuffers
                              && glMapBuffer
                              && glUnmapBuffer;

    yg::gl::g_isFramebufferSupported = glBindFramebuffer
                            && glFramebufferTexture2D
                            && glFramebufferRenderbuffer
                            && glGenFramebuffers
                            && glDeleteFramebuffers
                            && glCheckFramebufferStatusEXT
                            && glBlitFramebuffer;

    yg::gl::g_isRenderbufferSupported = glGenRenderbuffers
                             && glDeleteRenderbuffersEXT
                             && glBindRenderbufferEXT
                             && glRenderbufferStorageEXT;

    yg::gl::g_isMultisamplingSupported = (glRenderbufferStorageMultisample != NULL);
  }
} // namespace win32

#endif
