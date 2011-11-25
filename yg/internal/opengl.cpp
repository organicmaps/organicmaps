#include "opengl.hpp"

#include "../../base/logging.hpp"
#include "../../base/string_utils.hpp"
#include "../../std/bind.hpp"

#ifdef OMIM_OS_BADA
  #include <FBaseSys.h>
#endif

namespace yg
{
  namespace gl
  {
    bool HasExtension(const char *name)
    {
      string allExtensions(reinterpret_cast<char const * >(glGetString(GL_EXTENSIONS)));
      return allExtensions.find(name) != string::npos;
    }

    void DumpGLInformation()
    {
      LOG(LINFO, ("OpenGL Information"));
      LOG(LINFO, ("--------------------------------------------"));
      LOG(LINFO, ("Vendor     : ", glGetString(GL_VENDOR)));
      LOG(LINFO, ("Renderer   : ", glGetString(GL_RENDERER)));
      LOG(LINFO, ("Version    : ", glGetString(GL_VERSION)));

      vector<string> names;

      strings::Tokenize(string(reinterpret_cast<char const *>(glGetString(GL_EXTENSIONS))), " ", bind(&vector<string>::push_back, &names, _1));

      for (unsigned i = 0; i < names.size(); ++i)
      {
        if (i == 0)
          LOG(LINFO, ("Extensions : ", names[i]));
        else
          LOG(LINFO, ("             ", names[i]));
      }

      LOG(LINFO, ("--------------------------------------------"));
    }

    bool g_isBufferObjectsSupported = true;
    bool g_isFramebufferSupported = true;
    bool g_isRenderbufferSupported = true;

    void (* glBindBufferFn) (GLenum target, GLuint buffer);
    void (* glGenBuffersFn) (GLsizei n, GLuint *buffers);
    void (* glBufferDataFn) (GLenum target, long size, const GLvoid *data, GLenum usage);
    void (* glBufferSubDataFn) (GLenum target, long offset, long size, const GLvoid *data);
    void (* glDeleteBuffersFn) (GLsizei n, const GLuint *buffers);
    void* (* glMapBufferFn) (GLenum target, GLenum access);
    GLboolean (* glUnmapBufferFn) (GLenum target);

    void (* glBindFramebufferFn) (GLenum target, GLuint framebuffer);
    void (* glFramebufferTexture2DFn) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (* glFramebufferRenderbufferFn) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (* glGenFramebuffersFn) (GLsizei n, GLuint *framebuffers);
    void (* glDeleteFramebuffersFn) (GLsizei n, const GLuint *framebuffers);
    GLenum (* glCheckFramebufferStatusFn) (GLenum target);

    void (* glGenRenderbuffersFn) (GLsizei n, GLuint *renderbuffers);
    void (* glDeleteRenderbuffersFn) (GLsizei n, const GLuint *renderbuffers);
    void (* glBindRenderbufferFn) (GLenum target, GLuint renderbuffer);
    void (* glRenderbufferStorageFn) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

    bool g_doDeleteOnDestroy = true;

    void CheckExtensionSupport()
    {
      if (!(g_isFramebufferSupported && g_isRenderbufferSupported))
        throw platform_unsupported();
    }

    void LogError(char const * err, my::SrcPoint const & srcPt)
    {
      if (err)
      {
#ifdef OMIM_OS_BADA
        AppLog("%s", err);
#endif
        LOG(LERROR, (err, srcPt.FileName(), srcPt.Line()));
      }
    }

    char const * Error2String(int error)
    {
      switch (error)
      {
      case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
      case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
      case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
      case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
      case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
      case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#ifdef OMIM_OS_BADA        /* Errors / GetError return values */
      case EGL_SUCCESS : return 0;
      case EGL_NOT_INITIALIZED : return "EGL_NOT_INITIALIZED";
      case EGL_BAD_ACCESS : return "EGL_BAD_ACCESS";
      case EGL_BAD_ALLOC : return "EGL_BAD_ALLOC";
      case EGL_BAD_ATTRIBUTE : return "EGL_BAD_ATTRIBUTE";
      case EGL_BAD_CONFIG : return "EGL_BAD_CONFIG";
      case EGL_BAD_CONTEXT : return "EGL_BAD_CONTEXT";
      case EGL_BAD_CURRENT_SURFACE : return "EGL_BAD_CURRENT_SURFACE";
      case EGL_BAD_DISPLAY : return "EGL_BAD_DISPLAY";
      case EGL_BAD_MATCH : return "EGL_BAD_MATCH";
      case EGL_BAD_NATIVE_PIXMAP : return "EGL_BAD_NATIVE_PIXMAP";
      case EGL_BAD_NATIVE_WINDOW : return "EGL_BAD_NATIVE_WINDOW";
      case EGL_BAD_PARAMETER : return "EGL_BAD_PARAMETER";
      case EGL_BAD_SURFACE : return "EGL_BAD_SURFACE";
      case EGL_CONTEXT_LOST : return "EGL_CONTEXT_LOST";
#endif
      default: return 0;
      }
    }

    void CheckError(my::SrcPoint const & srcPt)
    {
      LogError(Error2String(glGetError()), srcPt);
    }

#ifdef OMIM_OS_BADA
    void CheckEGLError(my::SrcPoint const & srcPt)
    {
      LogError(Error2String(eglGetError()), srcPt);
    }
#endif
  }
}
