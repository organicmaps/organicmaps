#include "opengl.hpp"


namespace yg
{
  namespace gl
  {
    void InitExtensions()
    {
      DumpGLInformation();

      g_isBufferObjectsSupported = HasExtension("GL_ARB_vertex_buffer_object")
                                || HasExtension("GLX_ARB_vertex_buffer_object");

      glBindBufferFn = &glBindBuffer;
      glGenBuffersFn = &glGenBuffers;
      glBufferDataFn = &glBufferData;
      glBufferSubDataFn = &glBufferSubData;
      glDeleteBuffersFn = &glDeleteBuffers;
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
    }
  }
}

