#pragma once
#include "../../std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS)

  #include "../../std/windows.hpp"
  #include <gl/gl.h>

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
  #endif

#elif defined(OMIM_OS_ANDROID)
  #include <GLES/gl.h>
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
    extern bool g_isFramebufferSupported;
    extern bool g_isBufferObjectsSupported;
    extern bool g_isRenderbufferSupported;

    // buffer objects extensions

    extern const int GL_WRITE_ONLY_MWM;

    extern void (* glBindBufferFn) (GLenum target, GLuint buffer);
    extern void (* glGenBuffersFn) (GLsizei n, GLuint *buffers);
    extern void (* glBufferDataFn) (GLenum target, long size, const GLvoid *data, GLenum usage);
    extern void (* glBufferSubDataFn) (GLenum target, long offset, long size, const GLvoid *data);
    extern void (* glDeleteBuffersFn) (GLsizei n, const GLuint *buffers);
    extern void* (* glMapBufferFn) (GLenum target, GLenum access);
    extern GLboolean (* glUnmapBufferFn) (GLenum target);

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

    extern void (* glBindFramebufferFn) (GLenum target, GLuint framebuffer);
    extern void (* glFramebufferTexture2DFn) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    extern void (* glFramebufferRenderbufferFn) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    extern void (* glGenFramebuffersFn) (GLsizei n, GLuint *framebuffers);
    extern void (* glDeleteFramebuffersFn) (GLsizei n, const GLuint *framebuffers);
    extern GLenum (* glCheckFramebufferStatusFn) (GLenum target);

    // renderbuffer extensions


    extern void (* glGenRenderbuffersFn) (GLsizei n, GLuint *renderbuffers);
    extern void (* glDeleteRenderbuffersFn) (GLsizei n, const GLuint *renderbuffers);
    extern void (* glBindRenderbufferFn) (GLenum target, GLuint renderbuffer);
    extern void (* glRenderbufferStorageFn) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

    /// This flag controls, whether OpenGL resources should delete themselves upon destruction.
    /// Sounds odd, but in EGL there are cases when the only function one should call to finish
    /// its work with resources is eglTerminate, which by itself internally deletes all OpenGL resources.
    /// In this case we should set this variable to true to correctly deletes all our wrapper
    /// classes without calls to glDeleteXXX.

    extern bool g_doDeleteOnDestroy;

    /// each platform should have an implementation of this function
    /// to check extensions support and initialize function pointers.
    void InitExtensions();

    /// Does we have an extension with the specified name?
    bool HasExtension(char const * name);

    void DumpGLInformation();

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

