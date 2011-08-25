#pragma once
#include "../../std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS)
  #include "opengl_win32.hpp"

#elif defined(OMIM_OS_BADA)
  #include <FGraphicsOpengl.h>
  using namespace Osp::Graphics::Opengl;
  #define OMIM_GL_ES

#elif defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <TargetConditionals.h>

  #ifdef OMIM_OS_IPHONE
    #define USE_OPENGLES20_IF_AVAILABLE 0
    #include <OpenGLES/ES1/gl.h>
    #include <OpenGLES/ES1/glext.h>
    #define OMIM_GL_ES
  #else
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
  #endif

#elif defined(OMIM_OS_ANDROID)
  #include <GLES/gl.h>
  #include <GLES/glext.h>
  #define OMIM_GL_ES

#else
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif

#include "../../base/src_point.hpp"

namespace yg
{
  namespace gl
  {
    extern bool g_doFakeOpenGLCalls; //< for testing support
    extern bool g_isBufferObjectsSupported;
    extern bool g_isFramebufferSupported;
    extern bool g_isRenderbufferSupported;
    extern bool g_isMultisamplingSupported;

    /// return false to terminate program
    bool CheckExtensionSupport();

    void CheckError(my::SrcPoint const & srcPt);
    void CheckEGLError(my::SrcPoint const & srcPt);
  }
}

#ifdef DEBUG
#define OGLCHECK(f) do {f; yg::gl::CheckError(SRC()); } while(false)
#define OGLCHECKAFTER yg::gl::CheckError(SRC())
#define EGLCHECK do {yg::gl::CheckEGLError(SRC());} while(false)
#else
#define OGLCHECK(f) f
#define OGLCHECKAFTER
#define EGLCHECK
#endif
