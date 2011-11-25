// Intentionally no #pragma once

// buffer objects extensions

DEFINE_GL_PROC(PFNGLBINDBUFFERPROC, "glBindBufferFn", glBindBufferFn)
DEFINE_GL_PROC(PFNGLGENBUFFERSPROC, "glGenBuffers", glGenBuffersFn)
DEFINE_GL_PROC(PFNGLBUFFERDATAPROC, "glBufferData", glBufferDataFn)
DEFINE_GL_PROC(PFNGLBUFFERSUBDATAPROC, "glBufferSubData", glBufferSubDataFn)
DEFINE_GL_PROC(PFNGLDELETEBUFFERSPROC, "glDeleteBuffers", glDeleteBuffersFn)
DEFINE_GL_PROC(PFNGLMAPBUFFERPROC, "glMapBuffer", glMapBufferFn)
DEFINE_GL_PROC(PFNGLUNMAPBUFFERPROC, "glUnmapBuffer", glUnmapBufferFn)

// framebuffers extensions

DEFINE_GL_PROC(PFNGLBINDFRAMEBUFFERPROC, "glBindFramebuffer", glBindFramebufferFn)
DEFINE_GL_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, "glFramebufferTexture2D", glFramebufferTexture2DFn)
DEFINE_GL_PROC(PFNGLFRAMEBUFFERRENDERBUFFERPROC, "glFramebufferRenderbuffer", glFramebufferRenderbufferFn)
DEFINE_GL_PROC(PFNGLGENFRAMEBUFFERSPROC, "glGenFramebuffers", glGenFramebuffersFn)
DEFINE_GL_PROC(PFNGLDELETEFRAMEBUFFERSPROC, "glDeleteFramebuffers", glDeleteFramebuffersFn)
DEFINE_GL_PROC(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, "glCheckFramebufferStatus", glCheckFramebufferStatusFn)

// renderbuffer extensions

DEFINE_GL_PROC(PFNGLGENRENDERBUFFERSPROC, "glGenRenderbuffers", glGenRenderbuffersFn)
DEFINE_GL_PROC(PFNGLDELETERENDERBUFFERSEXTPROC, "glDeleteRenderbuffers", glDeleteRenderbuffersFn)
DEFINE_GL_PROC(PFNGLBINDRENDERBUFFEREXTPROC, "glBindRenderbuffer", glBindRenderbufferFn)
DEFINE_GL_PROC(PFNGLRENDERBUFFERSTORAGEEXTPROC, "glRenderbufferStorage", glRenderbufferStorageFn)
