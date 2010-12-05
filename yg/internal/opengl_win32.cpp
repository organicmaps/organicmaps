#include "../../base/SRC_FIRST.hpp"

#include "opengl_win32.hpp"

#include "../../base/assert.hpp"

#include "../../base/start_mem_debug.hpp"

#ifdef OMIM_OS_WINDOWS

PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;

namespace win32
{
  template <class TRet>
  TRet GetGLProc(HMODULE hInst, char const * name)
  {
    return reinterpret_cast<TRet>(::wglGetProcAddress(name));
  }

  void InitOpenGL()
  {
    HMODULE hInst = 0;

    glBindBuffer = GetGLProc<PFNGLBINDBUFFERPROC>(hInst, "glBindBuffer");
    glGenBuffers = GetGLProc<PFNGLGENBUFFERSPROC>(hInst, "glGenBuffers");
    glBufferData = GetGLProc<PFNGLBUFFERDATAPROC>(hInst, "glBufferData");
    glBufferSubData = GetGLProc<PFNGLBUFFERSUBDATAPROC>(hInst, "glBufferSubData");
    glDeleteBuffers = GetGLProc<PFNGLDELETEBUFFERSPROC>(hInst, "glDeleteBuffers");
    glActiveTexture = GetGLProc<PFNGLACTIVETEXTUREPROC>(hInst, "glActiveTexture");
    glClientActiveTexture = GetGLProc<PFNGLCLIENTACTIVETEXTUREPROC>(hInst, "glClientActiveTexture");
    glBindFramebuffer = GetGLProc<PFNGLBINDFRAMEBUFFERPROC>(hInst, "glBindFramebuffer");
    glFramebufferTexture2D = GetGLProc<PFNGLFRAMEBUFFERTEXTURE2DPROC>(hInst, "glFramebufferTexture2D");
    glFramebufferRenderbuffer = GetGLProc<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(hInst, "glFramebufferRenderbuffer");
    glGenFramebuffers = GetGLProc<PFNGLGENFRAMEBUFFERSPROC>(hInst, "glGenFramebuffers");
    glDeleteFramebuffers = GetGLProc<PFNGLDELETEFRAMEBUFFERSPROC>(hInst, "glDeleteFramebuffers");
    glBindRenderbufferEXT = GetGLProc<PFNGLBINDRENDERBUFFEREXTPROC>(hInst, "glBindRenderbufferEXT");
    glGenRenderbuffers = GetGLProc<PFNGLGENRENDERBUFFERSPROC>(hInst, "glGenRenderbuffers");
    glRenderbufferStorageEXT = GetGLProc<PFNGLRENDERBUFFERSTORAGEEXTPROC>(hInst, "glRenderbufferStorageEXT");
    glDeleteRenderbuffersEXT = GetGLProc<PFNGLDELETERENDERBUFFERSEXTPROC>(hInst, "glDeleteRenderbuffersEXT");
    glMapBuffer = GetGLProc<PFNGLMAPBUFFERPROC>(hInst, "glMapBuffer");
    glUnmapBuffer = GetGLProc<PFNGLUNMAPBUFFERPROC>(hInst, "glUnmapBuffer");
    glBlitFramebuffer = GetGLProc<PFNGLBLITFRAMEBUFFERPROC>(hInst, "glBlitFramebuffer");
    glRenderbufferStorageMultisample = GetGLProc<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC>(hInst, "glRenderbufferStorageMultisample");
  }
}

#endif
