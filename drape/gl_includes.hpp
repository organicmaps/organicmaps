#pragma once

#include "std/target_os.hpp"

#if defined(OMIM_OS_IPHONE)
#define GL_SILENCE_DEPRECATION
#include <OpenGLES/ES2/glext.h>
#include <OpenGLES/ES3/gl.h>
#elif defined(OMIM_OS_MAC)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#elif defined(OMIM_OS_WINDOWS)
#include "std/windows.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include "3party/GL/glext.h"
#elif defined(OMIM_OS_ANDROID)
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "android/sdk/src/main/cpp/app/organicmaps/sdk/opengl/gl3stub.h"
#elif defined(OMIM_OS_LINUX)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <dlfcn.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif
