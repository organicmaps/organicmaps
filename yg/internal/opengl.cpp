#include "opengl.hpp"

#include "../../base/logging.hpp"

#include "../../base/start_mem_debug.hpp"

#ifdef OMIM_OS_BADA
  #include <FBaseSys.h>
#endif

namespace yg
{
  namespace gl
  {
    bool g_isBufferObjectsSupported = true;
    bool g_isFramebufferSupported = true;
    bool g_isRenderbufferSupported = true;
    bool g_isMultisamplingSupported = true;

    bool CheckExtensionSupport()
    {
      /// this functionality must be supported
      return (g_isFramebufferSupported && g_isRenderbufferSupported);
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
