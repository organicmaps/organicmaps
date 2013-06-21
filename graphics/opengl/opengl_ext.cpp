#include "opengl.hpp"

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

    const GLenum GL_MODELVIEW_MWM = GL_MODELVIEW;
    const GLenum GL_PROJECTION_MWM = GL_PROJECTION;
    const GLenum GL_ALPHA_TEST_MWM = GL_ALPHA_TEST;

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

      g_isMapBufferSupported = g_isBufferObjectsSupported;

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

      glActiveTextureFn = &glActiveTexture;
      glGetAttribLocationFn = &glGetAttribLocation;
      glGetActiveAttribFn = &glGetActiveAttrib;
      glGetUniformLocationFn = &glGetUniformLocation;
      glGetActiveUniformFn = &glGetActiveUniform;
      glGetProgramInfoLogFn = &glGetProgramInfoLog;
      glGetProgramivFn = &glGetProgramiv;
      glLinkProgramFn = &glLinkProgram;
      glAttachShaderFn = &glAttachShader;
      glCreateProgramFn = &glCreateProgram;
      glDeleteProgramFn = & glDeleteProgram;
      glVertexAttribPointerFn = &glVertexAttribPointer;
      glEnableVertexAttribArrayFn = &glEnableVertexAttribArray;
      glUniformMatrix4fvFn = &glUniformMatrix4fv;
      glUniformMatrix3fvFn = &glUniformMatrix3fv;
      glUniformMatrix2fvFn = &glUniformMatrix2fv;
      glUniform4iFn = &glUniform4i;
      glUniform3iFn = &glUniform3i;
      glUniform2iFn = &glUniform2i;
      glUniform1iFn = &glUniform1i;
      glUniform4fFn = &glUniform4f;
      glUniform3fFn = &glUniform3f;
      glUniform2fFn = &glUniform2f;
      glUniform1fFn = &glUniform1f;
      glUseProgramFn = &glUseProgram;
      glGetShaderInfoLogFn = &glGetShaderInfoLog;
      glGetShaderivFn = &glGetShaderiv;
      glCompileShaderFn = &glCompileShader;
      // hack to avoid failed compilation on Mac OS X 10.8+
      typedef void (OPENGL_CALLING_CONVENTION * glShaderSourceFn_Type)(GLuint shader, GLsizei count, const GLchar ** string, const GLint *length);
      glShaderSourceFn = reinterpret_cast<glShaderSourceFn_Type>(&glShaderSource);
      glCreateShaderFn = &glCreateShader;
      glDeleteShaderFn = &glDeleteShader;
    }
  }
}

