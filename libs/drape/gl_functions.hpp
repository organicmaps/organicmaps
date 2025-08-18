#pragma once

#include "drape/drape_global.hpp"
#include "drape/gl_constants.hpp"
#include "drape/gl_extensions_list.hpp"

#include "base/src_point.hpp"

#include <string>
#include <thread>

class GLFunctions
{
public:
  static dp::ApiVersion CurrentApiVersion;
  static dp::GLExtensionsList ExtensionsList;

  static void Init(dp::ApiVersion apiVersion);

  static bool glHasExtension(std::string const & name);
  static void glClearColor(float r, float g, float b, float a);
  static void glClear(uint32_t clearBits);
  static void glViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
  static void glScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
  static void glFlush();
  static void glFinish();

  static void glFrontFace(glConst mode);
  static void glCullFace(glConst face);

  static void glStencilOpSeparate(glConst face, glConst sfail, glConst dpfail, glConst dppass);
  static void glStencilFuncSeparate(glConst face, glConst func, int ref, uint32_t mask);

  static void glPixelStore(glConst name, uint32_t value);

  static int32_t glGetInteger(glConst pname);
  /// target = { gl_const::GLArrayBuffer, gl_const::GLElementArrayBuffer }
  /// name = { gl_const::GLBufferSize, gl_const::GLBufferUsage }
  static int32_t glGetBufferParameter(glConst target, glConst name);

  static std::string glGetString(glConst pname);

  static int32_t glGetMaxLineWidth();

  static void glEnable(glConst mode);
  static void glDisable(glConst mode);
  static void glClearDepthValue(double depth);
  static void glDepthMask(bool needWriteToDepthBuffer);
  static void glDepthFunc(glConst depthFunc);
  static void glBlendEquation(glConst function);
  static void glBlendFunc(glConst srcFactor, glConst dstFactor);

  /// Debug Output
  static bool CanEnableDebugMessages();
  using TglDebugProc = void (*)(glConst source, glConst type, uint32_t id, glConst severity, int32_t length,
                                char const * message, void * userParam);
  static void glDebugMessageCallback(TglDebugProc messageCallback, void * userParam);
  static void glDebugMessageControl(glConst source, glConst type, glConst severity, int32_t count, uint32_t const * ids,
                                    uint8_t enabled);

  static void glLineWidth(uint32_t value);

  /// VAO support
  static uint32_t glGenVertexArray();
  static void glBindVertexArray(uint32_t vao);
  static void glDeleteVertexArray(uint32_t vao);

  /// VBO support
  static uint32_t glGenBuffer();
  /// target - type of buffer to bind. Look GLConst
  static void glBindBuffer(uint32_t vbo, glConst target);
  static void glDeleteBuffer(uint32_t vbo);
  /// usage - Look GLConst
  static void glBufferData(glConst target, uint32_t size, void const * data, glConst usage);
  static void glBufferSubData(glConst target, uint32_t size, void const * data, uint32_t offset);

  static void * glMapBuffer(glConst target, glConst access = gl_const::GLWriteOnly);
  static void glUnmapBuffer(glConst target);

  static void * glMapBufferRange(glConst target, uint32_t offset, uint32_t length, glConst access);
  static void glFlushMappedBufferRange(glConst target, uint32_t offset, uint32_t length);

  /// Shaders support
  static uint32_t glCreateShader(glConst type);
  static void glShaderSource(uint32_t shaderID, std::string const & src, std::string const & defines);
  static bool glCompileShader(uint32_t shaderID, std::string & errorLog);
  static void glDeleteShader(uint32_t shaderID);

  static uint32_t glCreateProgram();
  static void glAttachShader(uint32_t programID, uint32_t shaderID);
  static void glDetachShader(uint32_t programID, uint32_t shaderID);
  static bool glLinkProgram(uint32_t programID, std::string & errorLog);
  static void glDeleteProgram(uint32_t programID);

  static void glUseProgram(uint32_t programID);
  static int8_t glGetAttribLocation(uint32_t programID, std::string const & name);
  static void glBindAttribLocation(uint32_t programID, uint8_t index, std::string const & name);

  /// enable vertex attribute binding. To get attributeLocation need to call glGetAttributeLocation
  static void glEnableVertexAttribute(int32_t attributeLocation);
  /// Configure vertex attribute binding.
  /// attrLocation - attribute location in shader program
  /// count - specify number of components with "type" for generic attribute
  /// needNormalize - if "true" then OGL will map integer value on [-1 : 1] for signed of on [0 : 1]
  /// for unsigned
  ///                 if "false" it will direct convert to float type
  ///                 if "type" == GLFloat this parameter have no sense
  /// stride - how much bytes need to seek from current attribute value to get the second value
  /// offset - how much bytes need to seek from begin of currenct buffer to get first attribute
  /// value
  static void glVertexAttributePointer(int32_t attrLocation, uint32_t count, glConst type, bool needNormalize,
                                       uint32_t stride, uint32_t offset);

  static void glGetActiveUniform(uint32_t programID, uint32_t uniformIndex, int32_t * uniformSize, glConst * type,
                                 std::string & name);

  static int8_t glGetUniformLocation(uint32_t programID, std::string const & name);
  static void glUniformValuei(int8_t location, int32_t v);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4);
  static void glUniformValueiv(int8_t location, int32_t * v, uint32_t size);

  static void glUniformValuef(int8_t location, float v);
  static void glUniformValuef(int8_t location, float v1, float v2);
  static void glUniformValuef(int8_t location, float v1, float v2, float v3);
  static void glUniformValuef(int8_t location, float v1, float v2, float v3, float v4);
  static void glUniformValuefv(int8_t location, float * v, uint32_t size);

  static void glUniformMatrix4x4Value(int8_t location, float const * values);

  static uint32_t glGetCurrentProgram();

  static int32_t glGetProgramiv(uint32_t program, glConst paramName);

  // Textures support
  static void glActiveTexture(glConst texBlock);
  static uint32_t glGenTexture();
  static void glDeleteTexture(uint32_t id);
  static void glBindTexture(uint32_t textureID);
  static void glTexImage2D(int width, int height, glConst layout, glConst pixelType, void const * data);
  static void glTexSubImage2D(int x, int y, int width, int height, glConst layout, glConst pixelType,
                              void const * data);
  static void glTexParameter(glConst param, glConst value);

  // Draw support
  static void glDrawElements(glConst primitive, uint32_t sizeOfIndex, uint32_t indexCount, uint32_t startIndex = 0);
  static void glDrawArrays(glConst mode, int32_t first, uint32_t count);

  // FBO support
  static void glGenFramebuffer(uint32_t * fbo);
  static void glDeleteFramebuffer(uint32_t * fbo);
  static void glBindFramebuffer(uint32_t fbo);
  static void glFramebufferTexture2D(glConst attachment, glConst texture);
  static uint32_t glCheckFramebufferStatus();
};

void CheckGLError(base::SrcPoint const & src);

#ifdef DEBUG
#define GLCHECK(x)       \
  do                     \
  {                      \
    (x);                 \
    CheckGLError(SRC()); \
  }                      \
  while (false)
#define GLCHECKCALL()    \
  do                     \
  {                      \
    CheckGLError(SRC()); \
  }                      \
  while (false)
#else
#define GLCHECK(x) (x)
#define GLCHECKCALL()
#endif
