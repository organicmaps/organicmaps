#include "opengl.hpp"

#include "opengl_glsl_impl.hpp"

#include "../../base/matrix.hpp"
#include "../../platform/platform.hpp"

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

    const int GL_WRITE_ONLY_MWM = GL_WRITE_ONLY;

    void InitExtensions()
    {
      DumpGLInformation();

      glEnableFn = &glsl::glEnable;
      glDisableFn = &glsl::glDisable;
      glAlphaFuncFn = &glsl::glAlphaFunc;

      glVertexPointerFn = &glsl::glVertexPointer;
      glNormalPointerFn = &glsl::glNormalPointer;
      glTexCoordPointerFn = &glsl::glTexCoordPointer;
      glEnableClientStateFn = &glsl::glEnableClientState;
      glUseSharpGeometryFn = &glsl::glUseSharpGeometry;

      glMatrixModeFn = &glsl::glMatrixMode;
      glLoadIdentityFn = &glsl::glLoadIdentity;
      glLoadMatrixfFn = &glsl::glLoadMatrixf;
      glOrthoFn = &glsl::glOrtho;
      glDrawElementsFn = &glsl::glDrawElements;

      g_isBufferObjectsSupported = true;

      glBindBufferFn = &glBindBuffer;
      glGenBuffersFn = &glGenBuffers;
      glBufferDataFn = &glBufferData;
      glBufferSubDataFn = &glBufferSubData;
      glDeleteBuffersFn = &glDeleteBuffers;

      g_isMapBufferSupported = false; //true;

      glMapBufferFn = &glMapBuffer;
      glUnmapBufferFn = &glUnmapBuffer;

      g_isFramebufferSupported = HasExtension("GL_ARB_framebuffer_object");

      glBindFramebufferFn = &glBindFramebufferEXT;
      glFramebufferTexture2DFn = &glFramebufferTexture2DEXT;
      glFramebufferRenderbufferFn = &glFramebufferRenderbufferEXT;
      glGenFramebuffersFn = &glGenFramebuffersEXT;
      glDeleteFramebuffersFn = &glDeleteFramebuffersEXT;
      glCheckFramebufferStatusFn = &glCheckFramebufferStatusEXT;

      g_isRenderbufferSupported = g_isFramebufferSupported;

      glGenRenderbuffersFn = &glGenRenderbuffersEXT;
      glDeleteRenderbuffersFn = &glDeleteRenderbuffersEXT;
      glBindRenderbufferFn = &glBindRenderbufferEXT;
      glRenderbufferStorageFn = &glRenderbufferStorageEXT;

      g_isSeparateBlendFuncSupported = HasExtension("GL_EXT_blend_func_separate");
      glBlendFuncSeparateFn = &glBlendFuncSeparateEXT;
    }
  }
}


