#pragma once
#include "../../std/target_os.hpp"
#include "../../base/logging.hpp"

#include "../../base/logging.hpp"

#if defined(OMIM_OS_WINDOWS)
  #include "../../std/windows.hpp"
  #include <gl/gl.h>
  #define GL_GLEXT_PROTOTYPES
  #include "../../3party/GL/glext.h"

#elif defined(OMIM_OS_BADA)
  #include <FGraphicsOpengl.h>
  using namespace Osp::Graphics::Opengl;
  #define OMIM_GL_ES

#elif defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <TargetConditionals.h>

  #ifdef OMIM_OS_IPHONE
    #define USE_OPENGLES20_IF_AVAILABLE 0
    #include <OpenGLES/ES1/gl.h>
    #define OMIM_GL_ES
  #else
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
  #endif

#elif defined(OMIM_OS_ANDROID)
  #include <GLES/gl.h>
  #define OMIM_GL_ES

#else
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif


#ifdef OMIM_OS_WINDOWS
  #define OPENGL_CALLING_CONVENTION __stdcall
#else
  #define OPENGL_CALLING_CONVENTION
#endif


#include "../../base/src_point.hpp"
#include "../../std/exception.hpp"


namespace yg
{
  namespace gl
  {
    extern bool g_isFramebufferSupported;
    extern bool g_isBufferObjectsSupported;
    extern bool g_isRenderbufferSupported;
    extern bool g_isSeparateBlendFuncSupported;

    // buffer objects extensions

    extern const int GL_WRITE_ONLY_MWM;

    extern void (OPENGL_CALLING_CONVENTION * glBindBufferFn) (GLenum target, GLuint buffer);
    extern void (OPENGL_CALLING_CONVENTION * glGenBuffersFn) (GLsizei n, GLuint *buffers);
    extern void (OPENGL_CALLING_CONVENTION * glBufferDataFn) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
    extern void (OPENGL_CALLING_CONVENTION * glBufferSubDataFn) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteBuffersFn) (GLsizei n, const GLuint *buffers);
    extern void * (OPENGL_CALLING_CONVENTION * glMapBufferFn) (GLenum target, GLenum access);
    extern GLboolean (OPENGL_CALLING_CONVENTION * glUnmapBufferFn) (GLenum target);

    // framebuffers extensions

    extern const int GL_FRAMEBUFFER_BINDING_MWM;
    extern const int GL_FRAMEBUFFER_MWM;
    extern const int GL_FRAMEBUFFER_UNSUPPORTED_MWM;
    extern const int GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_MWM;
    extern const int GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_MWM;
    extern const int GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_MWM;
    extern const int GL_FRAMEBUFFER_COMPLETE_MWM;
    extern const int GL_DEPTH_ATTACHMENT_MWM;
    extern const int GL_COLOR_ATTACHMENT0_MWM;
    extern const int GL_RENDERBUFFER_MWM;
    extern const int GL_RENDERBUFFER_BINDING_MWM;
    extern const int GL_DEPTH_COMPONENT16_MWM;
    extern const int GL_DEPTH_COMPONENT24_MWM;
    extern const int GL_RGBA8_MWM;

    extern void (OPENGL_CALLING_CONVENTION * glBindFramebufferFn) (GLenum target, GLuint framebuffer);
    extern void (OPENGL_CALLING_CONVENTION * glFramebufferTexture2DFn) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    extern void (OPENGL_CALLING_CONVENTION * glFramebufferRenderbufferFn) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    extern void (OPENGL_CALLING_CONVENTION * glGenFramebuffersFn) (GLsizei n, GLuint *framebuffers);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteFramebuffersFn) (GLsizei n, const GLuint *framebuffers);
    extern GLenum (OPENGL_CALLING_CONVENTION * glCheckFramebufferStatusFn) (GLenum target);

    // renderbuffer extensions

    extern void (OPENGL_CALLING_CONVENTION * glGenRenderbuffersFn) (GLsizei n, GLuint *renderbuffers);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteRenderbuffersFn) (GLsizei n, const GLuint *renderbuffers);
    extern void (OPENGL_CALLING_CONVENTION * glBindRenderbufferFn) (GLenum target, GLuint renderbuffer);
    extern void (OPENGL_CALLING_CONVENTION * glRenderbufferStorageFn) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

    // separate alpha blending extension

    extern void (OPENGL_CALLING_CONVENTION * glBlendFuncSeparateFn) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

    /// This flag controls, whether OpenGL resources should delete themselves upon destruction.
    /// Sounds odd, but in EGL there are cases when the only function one should call to finish
    /// its work with resources is eglTerminate, which by itself internally deletes all OpenGL resources.
    /// In this case we should set this variable to true to correctly deletes all our wrapper
    /// classes without calls to glDeleteXXX.

    extern bool g_doDeleteOnDestroy;

    /// This flag controls, whether the OGLCHECK macroses should log OGL calls.
    /// This is for debugging purpose only.

    extern bool g_doLogOGLCalls;

    /// each platform should have an implementation of this function
    /// to check extensions support and initialize function pointers.
    void InitExtensions();

    /// Does we have an extension with the specified name?
    bool HasExtension(char const * name);

    void DumpGLInformation();

    /// return false to terminate program
    /// @throws platform_unsupported - is the platform we are running on is unsupported.
    void CheckExtensionSupport();

    struct platform_unsupported : public exception
    {
      string m_reason;
      const char * what() const throw();
      platform_unsupported(char const * reason);
      ~platform_unsupported() throw();
    };

    void CheckError(my::SrcPoint const & srcPt);
    void CheckEGLError(my::SrcPoint const & srcPt);

  }
}

#define OMIM_GL_ENABLE_TRACE 1

#ifdef DEBUG
  #ifdef OMIM_GL_ENABLE_TRACE
    #define OGLCHECK(f) do { if (yg::gl::g_doLogOGLCalls) LOG(LDEBUG, (#f)); f; yg::gl::CheckError(SRC()); } while(false)
    #define OGLCHECKAFTER if (yg::gl::g_doLogOGLCalls) LOG(LDEBUG, ("OGLCHECKAFTER")); yg::gl::CheckError(SRC())
    #define EGLCHECK do { LOG(LDEBUG, ("EGLCHECK")); yg::gl::CheckEGLError(SRC()); } while(false)
  #else
    #define OGLCHECK(f) do { f; yg::gl::CheckError(SRC()); } while(false)
    #define OGLCHECKAFTER yg::gl::CheckError(SRC())
    #define EGLCHECK do { yg::gl::CheckEGLError(SRC()); } while(false)
  #endif

#else
  #define OGLCHECK(f) f
  #define OGLCHECKAFTER
  #define EGLCHECK

#endif
