#pragma once

#if defined(__APPLE__)
  #include <TargetConditionals.h>
  #if (TARGET_OS_IPHONE > 0)
    #define USE_OPENGLES20_IF_AVAILABLE 1
    #include <OpenGLES/ES2/gl.h>
    #define OGL_IOS
  #else
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
    #define OGL_MAC
  #endif
#elif
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>
  #define OGL_ANDROID
#endif

void CheckGLError();

#define GLCHECK(x) do { (x); CheckGLError(); } while (false)
#define GLCHECKCALL() do { CheckGLError(); } while (false)
