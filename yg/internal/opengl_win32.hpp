#pragma once

#include "../../std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS

#include "../../std/windows.hpp"
#include <gl/GL.h>
#include "../../3party/GL/glext.h"

#define DEFINE_GL_PROC(type, name) \
  extern type name;
#include "gl_procedures.inl"
#undef DEFINE_GL_PROC

namespace yg
{
  namespace gl
  {
    extern bool g_isBufferObjectsSupported;
    extern bool g_isFramebufferSupported;
    extern bool g_isRenderbufferSupported;
    extern bool g_isMultisamplingSupported;
  }
}

namespace win32
{
  void InitOpenGL();
}

#endif
