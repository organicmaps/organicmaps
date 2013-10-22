// Intentionally no #pragma once

// buffer objects extensions

DEFINE_GL_RPOC("glFlush", glFlushFn);
DEFINE_GL_PROC("glActiveTexture", glActiveTextureFn)
DEFINE_GL_PROC("glGetAttribLocation", glGetAttribLocationFn)
DEFINE_GL_PROC("glGetActiveAttrib", glGetActiveAttribFn)
DEFINE_GL_PROC("glGetActiveUniform", glGetActiveUniformFn)
DEFINE_GL_PROC("glGetUniformLocation", glGetUniformLocationFn)
DEFINE_GL_PROC("glGetProgramInfoLog", glGetProgramInfoLogFn)
DEFINE_GL_PROC("glGetProgramiv", glGetProgramivFn)
DEFINE_GL_PROC("glLinkProgram", glLinkProgramFn)
DEFINE_GL_PROC("glAttachShader", glAttachShaderFn)
DEFINE_GL_PROC("glCreateProgram", glCreateProgramFn)
DEFINE_GL_PROC("glDeleteProgram", glDeleteProgramFn)
DEFINE_GL_PROC("glVertexAttribPointer", glVertexAttribPointerFn)
DEFINE_GL_PROC("glEnableVertexAttribArray", glEnableVertexAttribArrayFn)
DEFINE_GL_PROC("glUniformMatrix4fv", glUniformMatrix4fvFn)
DEFINE_GL_PROC("glUniformMatrix3fv", glUniformMatrix3fvFn)
DEFINE_GL_PROC("glUniformMatrix2fv", glUniformMatrix2fvFn)
DEFINE_GL_PROC("glUniform4i", glUniform4iFn)
DEFINE_GL_PROC("glUniform3i", glUniform3iFn)
DEFINE_GL_PROC("glUniform2i", glUniform2iFn)
DEFINE_GL_PROC("glUniform1i", glUniform1iFn)
DEFINE_GL_PROC("glUniform4f", glUniform4fFn)
DEFINE_GL_PROC("glUniform3f", glUniform3fFn)
DEFINE_GL_PROC("glUniform2f", glUniform2fFn)
DEFINE_GL_PROC("glUniform1f", glUniform1fFn)
DEFINE_GL_PROC("glUseProgram", glUseProgramFn)
DEFINE_GL_PROC("glGetShaderInfoLog", glGetShaderInfoLogFn)
DEFINE_GL_PROC("glGetShaderiv", glGetShaderivFn)
DEFINE_GL_PROC("glCompileShader", glCompileShaderFn)
DEFINE_GL_PROC("glShaderSource", glShaderSourceFn)
DEFINE_GL_PROC("glCreateShader", glCreateShaderFn)
DEFINE_GL_PROC("glDeleteShader", glDeleteShaderFn)

DEFINE_GL_PROC("glBindBuffer", glBindBufferFn)
DEFINE_GL_PROC("glGenBuffers", glGenBuffersFn)
DEFINE_GL_PROC("glBufferData", glBufferDataFn)
DEFINE_GL_PROC("glBufferSubData", glBufferSubDataFn)
DEFINE_GL_PROC("glDeleteBuffers", glDeleteBuffersFn)
DEFINE_GL_PROC("glMapBuffer", glMapBufferFn)
DEFINE_GL_PROC("glUnmapBuffer", glUnmapBufferFn)

// framebuffers extensions

DEFINE_GL_PROC("glBindFramebuffer", glBindFramebufferFn)
DEFINE_GL_PROC("glFramebufferTexture2D", glFramebufferTexture2DFn)
DEFINE_GL_PROC("glFramebufferRenderbuffer", glFramebufferRenderbufferFn)
DEFINE_GL_PROC("glGenFramebuffers", glGenFramebuffersFn)
DEFINE_GL_PROC("glDeleteFramebuffers", glDeleteFramebuffersFn)
DEFINE_GL_PROC("glCheckFramebufferStatus", glCheckFramebufferStatusFn)

// renderbuffer extensions

DEFINE_GL_PROC("glGenRenderbuffers", glGenRenderbuffersFn)
DEFINE_GL_PROC("glDeleteRenderbuffers", glDeleteRenderbuffersFn)
DEFINE_GL_PROC("glBindRenderbuffer", glBindRenderbufferFn)
DEFINE_GL_PROC("glRenderbufferStorage", glRenderbufferStorageFn)
