// Intentionally no #pragma once

// buffer objects extensions

DEFINE_GL_PROC("glBindBufferFn", glBindBufferFn)
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
