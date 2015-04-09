#include "graphics/opengl/opengl.hpp"

//#include "../../3party/GL/glext.h"

#include "base/logging.hpp"


namespace graphics
{
  namespace gl
  {
    const int GL_FRAMEBUFFER_BINDING_MWM = GL_FRAMEBUFFER_BINDING_EXT;
    const int GL_FRAMEBUFFER_MWM = GL_FRAMEBUFFER_EXT;
    const int GL_FRAMEBUFFER_UNSUPPORTED_MWM = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
    const int GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_MWM = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
    const int GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_MWM = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT;
    const int GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_MWM = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
    const int GL_FRAMEBUFFER_COMPLETE_MWM = GL_FRAMEBUFFER_COMPLETE_EXT;

    const int GL_DEPTH_ATTACHMENT_MWM = GL_DEPTH_ATTACHMENT_EXT;
    const int GL_COLOR_ATTACHMENT0_MWM = GL_COLOR_ATTACHMENT0_EXT;
    const int GL_RENDERBUFFER_MWM = GL_RENDERBUFFER_EXT;
    const int GL_RENDERBUFFER_BINDING_MWM = GL_RENDERBUFFER_BINDING_EXT;
    const int GL_DEPTH_COMPONENT16_MWM = GL_DEPTH_COMPONENT16;
    const int GL_DEPTH_COMPONENT24_MWM = GL_DEPTH_COMPONENT24;
    const int GL_RGBA8_MWM = GL_RGBA8;
    const int GL_RGBA4_MWM = GL_RGBA4;

    const int GL_WRITE_ONLY_MWM = GL_WRITE_ONLY;

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
#include "graphics/opengl/gl_procedures.inl"
#undef DEFINE_GL_PROC

      // glFlush is a globally defined function
      glFlushFn = &glFlush;

      graphics::gl::g_isBufferObjectsSupported = glBindBufferFn
                                        && glGenBuffersFn
                                        && glBufferDataFn
                                        && glBufferSubDataFn
                                        && glDeleteBuffersFn;

      graphics::gl::g_isMapBufferSupported =  glMapBufferFn
                                        && glUnmapBufferFn;

      graphics::gl::g_isFramebufferSupported = glBindFramebufferFn
                                      && glFramebufferTexture2DFn
                                      && glFramebufferRenderbufferFn
                                      && glGenFramebuffersFn
                                      && glDeleteFramebuffersFn
                                      && glCheckFramebufferStatusFn;

      graphics::gl::g_isRenderbufferSupported = glGenRenderbuffersFn
                                       && glDeleteRenderbuffersFn
                                       && glBindRenderbufferFn
                                       && glRenderbufferStorageFn;

      graphics::gl::g_isSeparateBlendFuncSupported = false;
    }
  }
} // namespace win32
