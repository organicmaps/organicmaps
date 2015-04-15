#pragma once

#include "std/target_os.hpp"

#if defined(OMIM_OS_IPHONE)
  #define USE_OPENGLES20_IF_AVAILABLE 1
  #include <OpenGLES/ES2/gl.h>
  #include <OpenGLES/ES2/glext.h>
#elif defined(OMIM_OS_MAC)
  #include <OpenGL/gl.h>
  #include <OpenGL/glext.h>
#elif defined(OMIM_OS_WINDOWS)
  #include "std/windows.hpp"
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include "3party/GL/glext.h"
#elif defined(OMIM_OS_ANDROID)
  #include <EGL/egl.h>
  #include <GLES2/gl2.h>
  #define GL_GLEXT_PROTOTYPES
  #include <GLES2/gl2ext.h>
#else
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif
