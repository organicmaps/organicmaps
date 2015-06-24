#pragma once

#include "base/logging.hpp"

#include "std/target_os.hpp"


#if defined(OMIM_OS_WINDOWS)
  #include "../../std/windows.hpp"
  #include <gl/gl.h>
  #define GL_GLEXT_PROTOTYPES
  #include "../../3party/GL/glext.h"

#elif defined(OMIM_OS_TIZEN)
  #include <FGraphicsOpengl2.h>
  using namespace Tizen::Graphics::Opengl;
  #define OMIM_GL_ES

#elif defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <TargetConditionals.h>

  #ifdef OMIM_OS_IPHONE
    #define USE_OPENGLES20_IF_AVAILABLE 1
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
    #define OMIM_GL_ES
  #else
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
  #endif

#elif defined(OMIM_OS_ANDROID)

  #include <GLES2/gl2.h>
  #define GL_GLEXT_PROTOTYPES
  #include <GLES2/gl2ext.h>
  #define OMIM_GL_ES

#elif defined(OMIM_OS_MAEMO)
  #define USE_OPENGLES20_IF_AVAILABLE 1
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
  #define OMIM_GL_ES

#elif defined(OMIM_OS_TIZEN)
  #include <osp/gl2.h>
  #define GL_GLEXT_PROTOTYPES
  #include <osp/gl2ext.h>
  #define OMIM_GL_ES

#else
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>

#endif


#if defined(OMIM_OS_WINDOWS)
  #define OPENGL_CALLING_CONVENTION __stdcall
#elif defined(OMIM_OS_ANDROID)
  #define OPENGL_CALLING_CONVENTION __NDK_FPABI__
#else
  #define OPENGL_CALLING_CONVENTION
#endif

#include "base/src_point.hpp"
#include "std/exception.hpp"


namespace graphics
{
  namespace gl
  {
    // basic opengl functions and constants
    extern void (OPENGL_CALLING_CONVENTION * glEnableFn)(GLenum cap);
    extern void (OPENGL_CALLING_CONVENTION * glDisableFn)(GLenum cap);
    extern void (OPENGL_CALLING_CONVENTION * glAlphaFuncFn)(GLenum func, GLclampf ref);
    extern void (OPENGL_CALLING_CONVENTION * glEnableClientStateFn) (GLenum array);
    extern void (OPENGL_CALLING_CONVENTION * glVertexPointerFn) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    extern void (OPENGL_CALLING_CONVENTION * glNormalPointerFn) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    extern void (OPENGL_CALLING_CONVENTION * glTexCoordPointerFn) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

    /// this is a quickfix for sharpness of straight texts and symbols.
    /// should be refactored to something more consistent.
    extern void (OPENGL_CALLING_CONVENTION * glUseSharpGeometryFn) (GLboolean flag);

    extern const GLenum GL_VERTEX_ARRAY_MWM;
    extern const GLenum GL_TEXTURE_COORD_ARRAY_MWM;
    extern const GLenum GL_NORMAL_ARRAY_MWM;

    extern void (OPENGL_CALLING_CONVENTION * glMatrixModeFn) (GLenum mode);
    extern void (OPENGL_CALLING_CONVENTION * glLoadIdentityFn)();
    extern void (OPENGL_CALLING_CONVENTION * glLoadMatrixfFn) (const GLfloat *m);

    extern void (OPENGL_CALLING_CONVENTION * glOrthoFn) (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
    extern void (OPENGL_CALLING_CONVENTION * glDrawElementsFn) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    extern void (OPENGL_CALLING_CONVENTION * glFlushFn)();

    extern const GLenum GL_MODELVIEW_MWM;
    extern const GLenum GL_PROJECTION_MWM;

    extern const GLenum GL_ALPHA_TEST_MWM;

    /// information about supported extensions

    extern bool g_isMapBufferSupported;
    extern bool g_isBufferObjectsSupported;
    extern bool g_isFramebufferSupported;
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
    extern const int GL_RGBA4_MWM;

    extern void (OPENGL_CALLING_CONVENTION * glBindFramebufferFn) (GLenum target, GLuint framebuffer);
    extern void (OPENGL_CALLING_CONVENTION * glFramebufferTexture2DFn) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    extern void (OPENGL_CALLING_CONVENTION * glFramebufferRenderbufferFn) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    extern void (OPENGL_CALLING_CONVENTION * glGenFramebuffersFn) (GLsizei n, GLuint *framebuffers);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteFramebuffersFn) (GLsizei n, const GLuint *framebuffers);
    extern GLenum (OPENGL_CALLING_CONVENTION * glCheckFramebufferStatusFn) (GLenum target);
    extern void (OPENGL_CALLING_CONVENTION * glDiscardFramebufferFn)(GLenum target, GLsizei numAttachments, GLenum const * attachments);

    // renderbuffer extensions

    extern void (OPENGL_CALLING_CONVENTION * glGenRenderbuffersFn) (GLsizei n, GLuint *renderbuffers);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteRenderbuffersFn) (GLsizei n, const GLuint *renderbuffers);
    extern void (OPENGL_CALLING_CONVENTION * glBindRenderbufferFn) (GLenum target, GLuint renderbuffer);
    extern void (OPENGL_CALLING_CONVENTION * glRenderbufferStorageFn) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

    // separate alpha blending extension

    extern void (OPENGL_CALLING_CONVENTION * glBlendFuncSeparateFn) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

    extern void (OPENGL_CALLING_CONVENTION * glActiveTextureFn) (GLenum texture);
    extern GLint (OPENGL_CALLING_CONVENTION * glGetAttribLocationFn) (GLuint program, const GLchar *name);
    extern void (OPENGL_CALLING_CONVENTION * glGetActiveAttribFn)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    extern GLint (OPENGL_CALLING_CONVENTION * glGetUniformLocationFn)(GLuint program, const GLchar *name);
    extern void (OPENGL_CALLING_CONVENTION * glGetActiveUniformFn)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    extern void (OPENGL_CALLING_CONVENTION * glGetProgramInfoLogFn)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    extern void (OPENGL_CALLING_CONVENTION * glGetProgramivFn)(GLuint program, GLenum pname, GLint *params);
    extern void (OPENGL_CALLING_CONVENTION * glLinkProgramFn)(GLuint program);
    extern void (OPENGL_CALLING_CONVENTION * glAttachShaderFn)(GLuint program, GLuint shader);
    extern GLuint (OPENGL_CALLING_CONVENTION * glCreateProgramFn)(void);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteProgramFn)(GLuint program);
    extern void (OPENGL_CALLING_CONVENTION * glVertexAttribPointerFn)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
    extern void (OPENGL_CALLING_CONVENTION * glEnableVertexAttribArrayFn)(GLuint index);
    extern void (OPENGL_CALLING_CONVENTION * glUniformMatrix4fvFn)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    extern void (OPENGL_CALLING_CONVENTION * glUniformMatrix3fvFn)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    extern void (OPENGL_CALLING_CONVENTION * glUniformMatrix2fvFn)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
    extern void (OPENGL_CALLING_CONVENTION * glUniform4iFn)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    extern void (OPENGL_CALLING_CONVENTION * glUniform3iFn)(GLint location, GLint v0, GLint v1, GLint v2);
    extern void (OPENGL_CALLING_CONVENTION * glUniform2iFn)(GLint location, GLint v0, GLint v1);
    extern void (OPENGL_CALLING_CONVENTION * glUniform1iFn)(GLint location, GLint v0);
    extern void (OPENGL_CALLING_CONVENTION * glUniform4fFn)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    extern void (OPENGL_CALLING_CONVENTION * glUniform3fFn)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    extern void (OPENGL_CALLING_CONVENTION * glUniform2fFn)(GLint location, GLfloat v0, GLfloat v1);
    extern void (OPENGL_CALLING_CONVENTION * glUniform1fFn)(GLint location, GLfloat v0);
    extern void (OPENGL_CALLING_CONVENTION * glUseProgramFn)(GLuint program);
    extern void (OPENGL_CALLING_CONVENTION * glGetShaderInfoLogFn)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    extern void (OPENGL_CALLING_CONVENTION * glGetShaderivFn)(GLuint shader, GLenum pname, GLint *params);
    extern void (OPENGL_CALLING_CONVENTION * glCompileShaderFn)(GLuint shader);
    extern void (OPENGL_CALLING_CONVENTION * glShaderSourceFn)(GLuint shader, GLsizei count, const GLchar ** string, const GLint * length);
    extern GLuint (OPENGL_CALLING_CONVENTION * glCreateShaderFn)(GLenum type);
    extern void (OPENGL_CALLING_CONVENTION * glDeleteShaderFn)(GLuint shader);

    /// This flag controls, whether OpenGL resources should delete themselves upon destruction.
    /// Sounds odd, but in EGL there are cases when the only function one should call to finish
    /// its work with resources is eglTerminate, which by itself internally deletes all OpenGL resources.
    /// In this case we should set this variable to true to correctly deletes all our wrapper
    /// classes without calls to glDeleteXXX.

    extern bool g_hasContext;

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

  }
}

//#define OMIM_GL_ENABLE_TRACE 1

#ifdef DEBUG
  #ifdef OMIM_GL_ENABLE_TRACE
    #define OGLCHECK(f) do { if (graphics::gl::g_doLogOGLCalls) LOG(LDEBUG, (#f)); if (graphics::gl::g_hasContext) {f; graphics::gl::CheckError(SRC());} else LOG(LDEBUG, ("no OGL context. skipping OGL call")); } while(false)
    #define OGLCHECKAFTER if (graphics::gl::g_doLogOGLCalls) LOG(LDEBUG, ("OGLCHECKAFTER")); graphics::gl::CheckError(SRC())
  #else
    #define OGLCHECK(f) do { if (graphics::gl::g_hasContext) {f; graphics::gl::CheckError(SRC());} else LOG(LDEBUG, ("no OGL context. skipping OGL call.")); } while(false)
    #define OGLCHECKAFTER graphics::gl::CheckError(SRC())
  #endif

#else
#define OGLCHECK(f) do {if (graphics::gl::g_hasContext) {f;} else LOG(LDEBUG, ("no OGL context. skipping OGL call."));} while (false)
#define OGLCHECKAFTER
#endif
