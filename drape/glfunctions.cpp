#include "drape/glfunctions.hpp"
#include "drape/glIncludes.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#ifdef DEBUG
#include "base/thread.hpp"
#include "base/mutex.hpp"
#include "std/map.hpp"
#endif

#include "std/cstring.hpp"

#if defined(OMIM_OS_WINDOWS)
#define APIENTRY __stdcall
#elif defined(OMIM_OS_ANDROID)
#define APIENTRY __NDK_FPABI__
#else
#define APIENTRY
#endif

namespace
{
#ifdef DEBUG
  typedef pair<threads::ThreadID, glConst> TKey;
  typedef pair<TKey, uint32_t> TNode;
  typedef map<TKey, uint32_t> TBoundMap;
  TBoundMap g_boundBuffers;
  threads::Mutex g_mutex;
#endif

  inline GLboolean convert(bool v)
  {
    return (v == true) ? GL_TRUE : GL_FALSE;
  }

  void (APIENTRY *glClearColorFn)(GLfloat r, GLfloat g, GLfloat b, GLfloat a)                                      = NULL;
  void (APIENTRY *glClearFn)(GLbitfield mask)                                                                      = NULL;
  void (APIENTRY *glViewportFn)(GLint x, GLint y, GLsizei w, GLsizei h)                                            = NULL;
  void (APIENTRY *glFlushFn)()                                                                                     = NULL;

  void (APIENTRY *glBindFramebufferFn)(GLenum target, GLuint id)                                                   = NULL;

  /// VAO
  void (APIENTRY *glGenVertexArraysFn)(GLsizei n, GLuint * ids)                                                    = NULL;
  void (APIENTRY *glBindVertexArrayFn)(GLuint id)                                                                  = NULL;
  void (APIENTRY *glDeleteVertexArrayFn)(GLsizei n, GLuint const * ids)                                            = NULL;

  /// VBO
  void (APIENTRY *glGenBuffersFn)(GLsizei n, GLuint * buffers)                                                     = NULL;
  void (APIENTRY *glBindBufferFn)(GLenum target, GLuint buffer)                                                    = NULL;
  void (APIENTRY *glDeleteBuffersFn)(GLsizei n, GLuint const * buffers)                                            = NULL;
  void (APIENTRY *glBufferDataFn)(GLenum target, GLsizeiptr size, GLvoid const * data, GLenum usage)               = NULL;
  void (APIENTRY *glBufferSubDataFn)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid const * data)         = NULL;
  void * (APIENTRY *glMapBufferFn)(GLenum target, GLenum access)                                                   = NULL;
  GLboolean (APIENTRY *glUnmapBufferFn)(GLenum target)                                                             = NULL;

  /// Shaders
  GLuint (APIENTRY *glCreateShaderFn)(GLenum type)                                                                 = NULL;
  void (APIENTRY *glShaderSourceFn)(GLuint shaderID, GLsizei count, GLchar const ** string, GLint const * length)  = NULL;
  void (APIENTRY *glCompileShaderFn)(GLuint shaderID)                                                              = NULL;
  void (APIENTRY *glDeleteShaderFn)(GLuint shaderID)                                                               = NULL;
  void (APIENTRY *glGetShaderivFn)(GLuint shaderID, GLenum name, GLint * p)                                        = NULL;
  void (APIENTRY *glGetShaderInfoLogFn)(GLuint shaderID, GLsizei maxLength, GLsizei * length, GLchar * infoLog)    = NULL;

  GLuint (APIENTRY *glCreateProgramFn)()                                                                           = NULL;
  void (APIENTRY *glAttachShaderFn)(GLuint programID, GLuint shaderID)                                             = NULL;
  void (APIENTRY *glDetachShaderFn)(GLuint programID, GLuint shaderID)                                             = NULL;
  void (APIENTRY *glLinkProgramFn)(GLuint programID)                                                               = NULL;
  void (APIENTRY *glDeleteProgramFn)(GLuint programID)                                                             = NULL;
  void (APIENTRY *glGetProgramivFn)(GLuint programID, GLenum name, GLint * p)                                      = NULL;
  void (APIENTRY *glGetProgramInfoLogFn)(GLuint programID, GLsizei maxLength, GLsizei * length, GLchar * infoLog)  = NULL;

  void (APIENTRY *glUseProgramFn)(GLuint programID)                                                                = NULL;
  GLint (APIENTRY *glGetAttribLocationFn)(GLuint program, GLchar const * name)                                     = NULL;
  void (APIENTRY *glBindAttribLocationFn)(GLuint program, GLuint index, GLchar const * name)                       = NULL;

  void (APIENTRY *glEnableVertexAttributeFn)(GLuint location)                                                      = NULL;
  void (APIENTRY *glVertexAttributePointerFn)(GLuint index,
                                              GLint count,
                                              GLenum type,
                                              GLboolean normalize,
                                              GLsizei stride,
                                              GLvoid const * p)                                                    = NULL;

  GLint (APIENTRY *glGetUniformLocationFn)(GLuint programID, GLchar const * name)                                  = NULL;

  void (APIENTRY *glGetActiveUniformFn)(GLuint programID,
                                        GLuint uniformIndex,
                                        GLsizei bufSize,
                                        GLsizei * length,
                                        GLint * size,
                                        GLenum * type,
                                        GLchar * name)                                                             = NULL;

  void (APIENTRY *glUniform1iFn)(GLint location, GLint value)                                                      = NULL;
  void (APIENTRY *glUniform2iFn)(GLint location, GLint v1, GLint v2)                                               = NULL;
  void (APIENTRY *glUniform3iFn)(GLint location, GLint v1, GLint v2, GLint v3)                                     = NULL;
  void (APIENTRY *glUniform4iFn)(GLint location, GLint v1, GLint v2, GLint v3, GLint v4)                           = NULL;
  void (APIENTRY *glUniform1ivFn)(GLint location, GLsizei count, GLint const * value)                              = NULL;

  void (APIENTRY *glUniform1fFn)(GLint location, GLfloat value)                                                    = NULL;
  void (APIENTRY *glUniform2fFn)(GLint location, GLfloat v1, GLfloat v2)                                           = NULL;
  void (APIENTRY *glUniform3fFn)(GLint location, GLfloat v1, GLfloat v2, GLfloat v3)                               = NULL;
  void (APIENTRY *glUniform4fFn)(GLint location, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4)                   = NULL;
  void (APIENTRY *glUniform1fvFn)(GLint location, GLsizei count, GLfloat const * value)                            = NULL;

  void (APIENTRY *glUniformMatrix4fvFn)(GLint location, GLsizei count, GLboolean transpose, GLfloat const * value) = NULL;

  int const GLCompileStatus = GL_COMPILE_STATUS;
  int const GLLinkStatus = GL_LINK_STATUS;
}

void GLFunctions::Init()
{
  /// VAO
#if defined(OMIM_OS_MAC)
  glGenVertexArraysFn = &glGenVertexArraysAPPLE;
  glBindVertexArrayFn = &glBindVertexArrayAPPLE;
  glDeleteVertexArrayFn = &glDeleteVertexArraysAPPLE;
  glMapBufferFn = &::glMapBuffer;
  glUnmapBufferFn = &::glUnmapBuffer;
#elif defined(OMIM_OS_LINUX)
  glGenVertexArraysFn = &::glGenVertexArrays;
  glBindVertexArrayFn = &::glBindVertexArray;
  glDeleteVertexArrayFn = &::glDeleteVertexArrays;
  glMapBufferFn = &::glMapBuffer;  // I don't know correct name for linux!
  glUnmapBufferFn = &::glUnmapBuffer; // I don't know correct name for linux!
#elif defined(OMIM_OS_ANDROID)
  typedef void (APIENTRY *glGenVertexArraysType)(GLsizei n, GLuint * arrays);
  typedef void (APIENTRY *glBindVertexArrayType)(GLuint array);
  typedef void (APIENTRY *glDeleteVertexArrayType)(GLsizei n, GLuint const * ids);
  glGenVertexArraysFn = (glGenVertexArraysType)eglGetProcAddress("glGenVertexArraysOES");
  glBindVertexArrayFn = (glBindVertexArrayType)eglGetProcAddress("glBindVertexArrayOES");
  glDeleteVertexArrayFn = (glDeleteVertexArrayType)eglGetProcAddress("glDeleteVertexArraysOES");
  glMapBufferFn = &::glMapBufferOES;
  glUnmapBufferFn = &::glUnmapBufferOES;
#elif defined(OMIM_OS_MOBILE)
  glGenVertexArraysFn = &glGenVertexArraysOES;
  glBindVertexArrayFn = &glBindVertexArrayOES;
  glDeleteVertexArrayFn = &glDeleteVertexArraysOES;
  glMapBufferFn = &::glMapBufferOES;
  glUnmapBufferFn = &::glUnmapBufferOES;
#endif

  glBindFramebufferFn = &::glBindFramebuffer;

  glClearColorFn = &::glClearColor;
  glClearFn = &::glClear;
  glViewportFn = &::glViewport;
  glFlushFn = &::glFlush;

  /// VBO
  glGenBuffersFn = &::glGenBuffers;
  glBindBufferFn = &::glBindBuffer;
  glDeleteBuffersFn = &::glDeleteBuffers;
  glBufferDataFn = &::glBufferData;
  glBufferSubDataFn = &::glBufferSubData;

  /// Shaders
  glCreateShaderFn = &::glCreateShader;
  typedef void (APIENTRY *glShaderSource_Type)(GLuint shaderID, GLsizei count, GLchar const ** string, GLint const * length);
  glShaderSourceFn = reinterpret_cast<glShaderSource_Type>(&::glShaderSource);
  glCompileShaderFn = &::glCompileShader;
  glDeleteShaderFn = &::glDeleteShader;
  glGetShaderivFn = &::glGetShaderiv;
  glGetShaderInfoLogFn = &::glGetShaderInfoLog;

  glCreateProgramFn = &::glCreateProgram;
  glAttachShaderFn = &::glAttachShader;
  glDetachShaderFn = &::glDetachShader;
  glLinkProgramFn = &::glLinkProgram;
  glDeleteProgramFn = &::glDeleteProgram;
  glGetProgramivFn = &::glGetProgramiv;
  glGetProgramInfoLogFn = &::glGetProgramInfoLog;

  glUseProgramFn = &::glUseProgram;
  glGetAttribLocationFn = &::glGetAttribLocation;
  glBindAttribLocationFn = &::glBindAttribLocation;

  glEnableVertexAttributeFn = &::glEnableVertexAttribArray;
  glVertexAttributePointerFn = &::glVertexAttribPointer;

  glGetUniformLocationFn = &::glGetUniformLocation;
  glGetActiveUniformFn = &::glGetActiveUniform;
  glUniform1iFn = &::glUniform1i;
  glUniform2iFn = &::glUniform2i;
  glUniform3iFn = &::glUniform3i;
  glUniform4iFn = &::glUniform4i;
  glUniform1ivFn = &::glUniform1iv;

  glUniform1fFn = &::glUniform1f;
  glUniform2fFn = &::glUniform2f;
  glUniform3fFn = &::glUniform3f;
  glUniform4fFn = &::glUniform4f;
  glUniform1fvFn = &::glUniform1fv;

  glUniformMatrix4fvFn = &glUniformMatrix4fv;
}

bool GLFunctions::glHasExtension(string const & name)
{
  char const* extensions = reinterpret_cast<char const * >(glGetString(GL_EXTENSIONS));
  char const * extName = name.c_str();
  char const * ptr = NULL;
  while ((ptr = strstr(extensions, extName)) != NULL)
  {
    char const * end = ptr + strlen(extName);
    if (isspace(*end) || *end == '\0')
        return true;

    extensions = end;
  }

  return false;
}

void GLFunctions::glClearColor(float r, float g, float b, float a)
{
  ASSERT(glClearColorFn != NULL, ());
  GLCHECK(glClearColorFn(r, g, b, a));
}

void GLFunctions::glClear()
{
  ASSERT(glClearFn != NULL, ());
  GLCHECK(glClearFn(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void GLFunctions::glClearDepth()
{
  ASSERT(glClearFn != NULL, ());
  GLCHECK(glClearFn(GL_DEPTH_BUFFER_BIT));
}

void GLFunctions::glViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  ASSERT(glViewportFn != NULL, ());
  GLCHECK(glViewportFn(x, y, w, h));
}

void GLFunctions::glFlush()
{
  ASSERT(glFlushFn != NULL, ());
  GLCHECK(glFlushFn());
}

void GLFunctions::glPixelStore(glConst name, uint32_t value)
{
  GLCHECK(::glPixelStorei(name, value));
}

int32_t GLFunctions::glGetInteger(glConst pname)
{
  GLint value;
  GLCHECK(::glGetIntegerv(pname, &value));
  return (int32_t)value;
}

void GLFunctions::glEnable(glConst mode)
{
  GLCHECK(::glEnable(mode));
}

void GLFunctions::glDisable(glConst mode)
{
  GLCHECK(::glDisable(mode));
}

void GLFunctions::glClearDepthValue(double depth)
{
#if defined(OMIM_OS_IPHONE) || defined(OMIM_OS_ANDROID)
  GLCHECK(::glClearDepthf(static_cast<GLclampf>(depth)));
#else
  GLCHECK(::glClearDepth(depth));
#endif
}

void GLFunctions::glDepthMask(bool needWriteToDepthBuffer)
{
  GLCHECK(::glDepthMask(convert(needWriteToDepthBuffer)));
}

void GLFunctions::glDepthFunc(glConst depthFunc)
{
  GLCHECK(::glDepthFunc(depthFunc));
}

void GLFunctions::glBlendEquation(glConst function)
{
  GLCHECK(::glBlendEquation(function));
}

void GLFunctions::glBlendFunc(glConst srcFactor, glConst dstFactor)
{
  GLCHECK(::glBlendFunc(srcFactor, dstFactor));
}

void GLFunctions::glBindFramebuffer(glConst target, uint32_t id)
{
  ASSERT(glBindFramebufferFn != NULL, ());
  GLCHECK(glBindFramebufferFn(target, id));
}

uint32_t GLFunctions::glGenVertexArray()
{
  ASSERT(glGenVertexArraysFn != NULL, ());
  GLuint result = 0;
  GLCHECK(glGenVertexArraysFn(1, &result));
  return result;
}

void GLFunctions::glBindVertexArray(uint32_t vao)
{
  ASSERT(glBindVertexArrayFn != NULL, ());
  GLCHECK(glBindVertexArrayFn(vao));
}

void GLFunctions::glDeleteVertexArray(uint32_t vao)
{
  ASSERT(glDeleteVertexArrayFn != NULL, ());
  GLCHECK(glDeleteVertexArrayFn(1, &vao));
}

uint32_t GLFunctions::glGenBuffer()
{
  ASSERT(glGenBuffersFn != NULL, ());
  GLuint result = (GLuint)-1;
  GLCHECK(glGenBuffersFn(1, &result));
  return result;
}

void GLFunctions::glBindBuffer(uint32_t vbo, uint32_t target)
{
  ASSERT(glBindBufferFn != NULL, ());
#ifdef DEBUG
  threads::MutexGuard guard(g_mutex);
  g_boundBuffers[make_pair(threads::GetCurrentThreadID(), target)] = vbo;
#endif
  GLCHECK(glBindBufferFn(target, vbo));
}

void GLFunctions::glDeleteBuffer(uint32_t vbo)
{
  ASSERT(glDeleteBuffersFn != NULL, ());
#ifdef DEBUG
  threads::MutexGuard guard(g_mutex);
  for (TNode const & n : g_boundBuffers)
    ASSERT(n.second != vbo, ());
#endif
  GLCHECK(glDeleteBuffersFn(1, &vbo));
}

void GLFunctions::glBufferData(glConst target, uint32_t size, void const * data, glConst usage)
{
  ASSERT(glBufferDataFn != NULL, ());
  GLCHECK(glBufferDataFn(target, size, data, usage));
}

void GLFunctions::glBufferSubData(glConst target, uint32_t size, void const * data, uint32_t offset)
{
  ASSERT(glBufferSubDataFn != NULL, ());
  GLCHECK(glBufferSubDataFn(target, offset, size, data));
}

void * GLFunctions::glMapBuffer(glConst target)
{
  ASSERT(glMapBufferFn != NULL, ());
  void * result = glMapBufferFn(target, gl_const::GLWriteOnly);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glUnmapBuffer(glConst target)
{
  ASSERT(glUnmapBufferFn != NULL, ());
  VERIFY(glUnmapBufferFn(target) == GL_TRUE, ());
  GLCHECKCALL();
}

uint32_t GLFunctions::glCreateShader(glConst type)
{
  ASSERT(glCreateShaderFn != NULL, ());
  GLuint result = glCreateShaderFn(type);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glShaderSource(uint32_t shaderID, string const & src)
{
  ASSERT(glShaderSourceFn != NULL, ());
  GLchar const * source = src.c_str();
  GLint length = src.size();
  GLCHECK(glShaderSourceFn(shaderID, 1, &source, &length));
}

bool GLFunctions::glCompileShader(uint32_t shaderID, string &errorLog)
{
  ASSERT(glCompileShaderFn != NULL, ());
  ASSERT(glGetShaderivFn != NULL, ());
  ASSERT(glGetShaderInfoLogFn != NULL, ());
  GLCHECK(glCompileShaderFn(shaderID));

  GLint result = GL_FALSE;
  GLCHECK(glGetShaderivFn(shaderID, GLCompileStatus, &result));
  if (result == GL_TRUE)
    return true;

  GLchar buf[1024];
  GLint length = 0;
  GLCHECK(glGetShaderInfoLogFn(shaderID, 1024, &length, buf));
  errorLog = string(buf, length);
  return false;
}

void GLFunctions::glDeleteShader(uint32_t shaderID)
{
  ASSERT(glDeleteShaderFn != NULL, ());
  GLCHECK(glDeleteBuffersFn(1, &shaderID));
}

uint32_t GLFunctions::glCreateProgram()
{
  ASSERT(glCreateProgramFn != NULL, ());
  GLuint result = glCreateProgramFn();
  GLCHECKCALL();
  return result;
}

void GLFunctions::glAttachShader(uint32_t programID, uint32_t shaderID)
{
  ASSERT(glAttachShaderFn != NULL, ());
  GLCHECK(glAttachShaderFn(programID, shaderID));
}

void GLFunctions::glDetachShader(uint32_t programID, uint32_t shaderID)
{
  ASSERT(glDetachShaderFn != NULL, ());
  GLCHECK(glDetachShaderFn(programID, shaderID));
}

bool GLFunctions::glLinkProgram(uint32_t programID, string & errorLog)
{
  ASSERT(glLinkProgramFn != NULL, ());
  ASSERT(glGetProgramivFn != NULL, ());
  ASSERT(glGetProgramInfoLogFn != NULL, ());
  GLCHECK(glLinkProgramFn(programID));

  GLint result = GL_FALSE;
  GLCHECK(glGetProgramivFn(programID, GLLinkStatus, &result));

  if (result == GL_TRUE)
    return true;

  GLchar buf[1024];
  GLint length = 0;
  GLCHECK(glGetProgramInfoLogFn(programID, 1024, &length, buf));
  errorLog = string(buf, length);
  return false;
}

void GLFunctions::glDeleteProgram(uint32_t programID)
{
  ASSERT(glDeleteProgramFn != NULL, ());
  GLCHECK(glDeleteProgramFn(programID));
}

void GLFunctions::glUseProgram(uint32_t programID)
{
  ASSERT(glUseProgramFn != NULL, ());
  GLCHECK(glUseProgramFn(programID));
}

int8_t GLFunctions::glGetAttribLocation(uint32_t programID, string const & name)
{
  ASSERT(glGetAttribLocationFn != NULL, ());
  int result = glGetAttribLocationFn(programID, name.c_str());
  GLCHECKCALL();
  ASSERT(result != -1, ());
  return result;
}

void GLFunctions::glBindAttribLocation(uint32_t programID, uint8_t index, string const & name)
{
  ASSERT(glBindAttribLocationFn != NULL, ());
  GLCHECK(glBindAttribLocationFn(programID, index, name.c_str()));
}

void GLFunctions::glEnableVertexAttribute(int attributeLocation)
{
  ASSERT(glEnableVertexAttributeFn != NULL, ());
  GLCHECK(glEnableVertexAttributeFn(attributeLocation));
}

void GLFunctions::glVertexAttributePointer(int attrLocation,
                              uint32_t count,
                              glConst type,
                              bool needNormalize,
                              uint32_t stride,
                              uint32_t offset)
{
  ASSERT(glVertexAttributePointerFn != NULL, ());
  GLCHECK(glVertexAttributePointerFn(attrLocation,
                                     count,
                                     type,
                                     convert(needNormalize),
                                     stride,
                                     reinterpret_cast<void *>(offset)));
}

void GLFunctions::glGetActiveUniform(uint32_t programID, uint32_t uniformIndex,
                                     int32_t * uniformSize, glConst * type, string & name)
{
  ASSERT(glGetActiveUniformFn != NULL, ());
  char buff[256];
  GLCHECK(glGetActiveUniformFn(programID, uniformIndex, 256, NULL, uniformSize, type, buff));
  name = buff;
}

int8_t GLFunctions::glGetUniformLocation(uint32_t programID, string const & name)
{
  ASSERT(glGetUniformLocationFn != NULL, ());
  int result = glGetUniformLocationFn(programID, name.c_str());
  GLCHECKCALL();
  ASSERT(result != -1, ());
  return result;
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v)
{
  ASSERT(glUniform1iFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1iFn(location, v));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2)
{
  ASSERT(glUniform2iFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform2iFn(location, v1, v2));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3)
{
  ASSERT(glUniform3iFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform3iFn(location, v1, v2, v3));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
  ASSERT(glUniform4iFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform4iFn(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformValueiv(int8_t location, int32_t * v, uint32_t size)
{
  ASSERT(glUniform1ivFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1ivFn(location, size, v));
}

void GLFunctions::glUniformValuef(int8_t location, float v)
{
  ASSERT(glUniform1fFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1fFn(location, v));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2)
{
  ASSERT(glUniform2fFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform2fFn(location, v1, v2));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3)
{
  ASSERT(glUniform3fFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform3fFn(location, v1, v2, v3));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3, float v4)
{
  ASSERT(glUniform4fFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform4fFn(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformValuefv(int8_t location, float * v, uint32_t size)
{
  ASSERT(glUniform1fvFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1fvFn(location, size, v));
}

void GLFunctions::glUniformMatrix4x4Value(int8_t location,  float const * values)
{
  ASSERT(glUniformMatrix4fvFn != NULL, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniformMatrix4fvFn(location, 1, GL_FALSE, values));
}

uint32_t GLFunctions::glGetCurrentProgram()
{
  GLint programIndex = 0;
  GLCHECK(glGetIntegerv(GL_CURRENT_PROGRAM, &programIndex));
  return programIndex;
}

int32_t GLFunctions::glGetProgramiv(uint32_t program, glConst paramName)
{
  ASSERT(glGetProgramivFn != NULL, ());
  GLint paramValue = 0;
  GLCHECK(glGetProgramivFn(program, paramName, &paramValue));
  return paramValue;
}

void GLFunctions::glActiveTexture(glConst texBlock)
{
  GLCHECK(::glActiveTexture(texBlock));
}

uint32_t GLFunctions::glGenTexture()
{
  GLuint result = 0;
  GLCHECK(::glGenTextures(1, &result));
  return result;
}

void GLFunctions::glDeleteTexture(uint32_t id)
{
  GLCHECK(::glDeleteTextures(1, &id));
}

void GLFunctions::glBindTexture(uint32_t textureID)
{
  GLCHECK(::glBindTexture(GL_TEXTURE_2D, textureID));
}

void GLFunctions::glTexImage2D(int width, int height, glConst layout, glConst pixelType, void const * data)
{
  GLCHECK(::glTexImage2D(GL_TEXTURE_2D, 0, layout, width, height, 0, layout, pixelType, data));
}

void GLFunctions::glTexSubImage2D(int x, int y, int width, int height, glConst layout, glConst pixelType, void const * data)
{
  GLCHECK(::glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, layout, pixelType, data));
}

void GLFunctions::glTexParameter(glConst param, glConst value)
{
  GLCHECK(::glTexParameteri(GL_TEXTURE_2D, param, value));
}

void GLFunctions::glDrawElements(uint16_t indexCount)
{
  GLCHECK(::glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0));
}

void CheckGLError()
{
  GLenum result = glGetError();
  while (result != GL_NO_ERROR)
  {
    LOG(LERROR, ("GLError:", result));
    result = glGetError();
  }
}
