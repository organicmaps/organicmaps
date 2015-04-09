#include "graphics/opengl/opengl.hpp"

#ifdef OMIM_OS_ANDROID
  #define GL_GLEXT_PROTOTYPES
  #include <GLES/glext.h>
#endif

#ifdef OMIM_OS_IPHONE
  #include <TargetConditionals.h>
  #include <OpenGLES/ES1/glext.h>
#endif

namespace yg
{
  namespace gl
  {
    const int GL_FRAMEBUFFER_BINDING_MWM = GL_FRAMEBUFFER_BINDING_OES;
    const int GL_FRAMEBUFFER_MWM = GL_FRAMEBUFFER_OES;
    const int GL_FRAMEBUFFER_UNSUPPORTED_MWM = GL_FRAMEBUFFER_UNSUPPORTED_OES;
    const int GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_MWM = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
    const int GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_MWM = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
    const int GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_MWM = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
    const int GL_FRAMEBUFFER_COMPLETE_MWM = GL_FRAMEBUFFER_COMPLETE_OES;

    const int GL_DEPTH_ATTACHMENT_MWM = GL_DEPTH_ATTACHMENT_OES;
    const int GL_COLOR_ATTACHMENT0_MWM = GL_COLOR_ATTACHMENT0_OES;
    const int GL_RENDERBUFFER_MWM = GL_RENDERBUFFER_OES;
    const int GL_RENDERBUFFER_BINDING_MWM = GL_RENDERBUFFER_BINDING_OES;
    const int GL_DEPTH_COMPONENT16_MWM = GL_DEPTH_COMPONENT16_OES;
    const int GL_DEPTH_COMPONENT24_MWM = GL_DEPTH_COMPONENT24_OES;
    const int GL_RGBA8_MWM = GL_RGBA8_OES;

    const int GL_WRITE_ONLY_MWM = GL_WRITE_ONLY_OES;

    const GLenum GL_MODELVIEW_MWM = GL_MODELVIEW;
    const GLenum GL_PROJECTION_MWM = GL_PROJECTION;
    const GLenum GL_ALPHA_TEST_MWM = GL_ALPHA_TEST;

    const GLenum GL_VERTEX_ARRAY_MWM = GL_VERTEX_ARRAY;
    const GLenum GL_TEXTURE_COORD_ARRAY_MWM = GL_TEXTURE_COORD_ARRAY;
    const GLenum GL_NORMAL_ARRAY_MWM = GL_NORMAL_ARRAY;

    void InitializeThread()
    {}

    void FinalizeThread()
    {}

    void glNormalPointerImpl(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
    {
      glNormalPointer(type, stride, pointer);
    }

    void glUseSharpGeometryImpl(GLboolean)
    {}

    void InitExtensions()
    {
      DumpGLInformation();

      glEnableFn = &glEnable;
      glDisableFn = &glDisable;
      glAlphaFuncFn = &glAlphaFunc;

      glVertexPointerFn = &glVertexPointer;
      glTexCoordPointerFn = &glTexCoordPointer;
      glNormalPointerFn = &glNormalPointerImpl;
      glEnableClientStateFn = &glEnableClientState;
      glUseSharpGeometryFn = &glUseSharpGeometryImpl;

      glMatrixModeFn = &glMatrixMode;
      glLoadIdentityFn = &glLoadIdentity;
      glLoadMatrixfFn = &glLoadMatrixf;
      glOrthoFn = &glOrthof;
      glDrawElementsFn = &glDrawElements;

      g_isMapBufferSupported = HasExtension("GL_OES_mapbuffer");

      glMapBufferFn = &glMapBufferOES;
      glUnmapBufferFn = &glUnmapBufferOES;

      /// generally speaking we should check for ARB_vertex_buffer extension
      g_isBufferObjectsSupported = g_isMapBufferSupported;

      glBindBufferFn = &glBindBuffer;
      glGenBuffersFn = &glGenBuffers;
      glBufferDataFn = &glBufferData;
      glBufferSubDataFn = &glBufferSubData;
      glDeleteBuffersFn = &glDeleteBuffers;

      g_isFramebufferSupported = HasExtension("GL_OES_framebuffer_object");

      glBindFramebufferFn = &glBindFramebufferOES;
      glFramebufferTexture2DFn = &glFramebufferTexture2DOES;
      glFramebufferRenderbufferFn = &glFramebufferRenderbufferOES;
      glGenFramebuffersFn = &glGenFramebuffersOES;
      glDeleteFramebuffersFn = &glDeleteFramebuffersOES;
      glCheckFramebufferStatusFn = &glCheckFramebufferStatusOES;

      g_isRenderbufferSupported = g_isFramebufferSupported;

      glGenRenderbuffersFn = &glGenRenderbuffersOES;
      glDeleteRenderbuffersFn = &glDeleteRenderbuffersOES;
      glBindRenderbufferFn = &glBindRenderbufferOES;
      glRenderbufferStorageFn = &glRenderbufferStorageOES;

      g_isSeparateBlendFuncSupported = HasExtension("GL_OES_blend_func_separate");
      glBlendFuncSeparateFn = &glBlendFuncSeparateOES;
    }
  }
}
