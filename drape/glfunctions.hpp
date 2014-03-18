#pragma once

#include "glconstants.hpp"
#include "../std/string.hpp"

class GLFunctions
{
public:
  static void Init();

  static bool glHasExtension(const string & name);
  static void glClearColor(float r, float g, float b, float a);
  static void glClear();
  static void glViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
  static void glFlush();

  static void glEnable(glConst mode);
  static void glDisable(glConst mode);
  static void glClearDepth(double depth);
  static void glDepthMask(bool needWriteToDepthBuffer);
  static void glDepthFunc(glConst depthFunc);

  static void glBindFramebuffer(glConst target, uint32_t id);

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
  static void glBufferData(glConst target, uint32_t size, const void * data, glConst usage);
  static void glBufferSubData(glConst target, uint32_t size, const void *data, uint32_t offset);

  /// Shaders support
  static uint32_t glCreateShader(glConst type);
  static void glShaderSource(uint32_t shaderID, const string & src);
  static bool glCompileShader(uint32_t shaderID, string & errorLog);
  static void glDeleteShader(uint32_t shaderID);

  static uint32_t glCreateProgram();
  static void glAttachShader(uint32_t programID, uint32_t shaderID);
  static void glDetachShader(uint32_t programID, uint32_t shaderID);
  static bool glLinkProgram(uint32_t programID, string & errorLog);
  static void glDeleteProgram(uint32_t programID);

  static void glUseProgram(uint32_t programID);
  static int8_t glGetAttribLocation(uint32_t programID, const string & name);
  static void glBindAttribLocation(uint32_t programID, uint8_t index, const string & name);

  /// enable vertex attribute binding. To get attributeLocation need to call glGetAttributeLocation
  static void glEnableVertexAttribute(int32_t attributeLocation);
  /// Configure vertex attribute binding.
  /// attrLocation - attribute location in shader program
  /// count - specify number of components with "type" for generic attribute
  /// needNormalize - if "true" then OGL will map integer value on [-1 : 1] for signed of on [0 : 1] for unsigned
  ///                 if "false" it will direct convert to float type
  ///                 if "type" == GLFloat this parameter have no sense
  /// stride - how much bytes need to seek from current attribute value to get the second value
  /// offset - how much bytes need to seek from begin of currenct buffer to get first attribute value
  static void glVertexAttributePointer(int32_t attrLocation,
                                       uint32_t count,
                                       glConst type,
                                       bool needNormalize,
                                       uint32_t stride,
                                       uint32_t offset);

  static void glGetActiveUniform(uint32_t programID, uint32_t uniformIndex,
                                 int32_t * uniformSize, glConst * type, string & name);

  static int8_t glGetUniformLocation(uint32_t programID, const string & name);
  static void glUniformValuei(int8_t location, int32_t v);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3);
  static void glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  static void glUniformValuef(int8_t location, float v);
  static void glUniformValuef(int8_t location, float v1, float v2);
  static void glUniformValuef(int8_t location, float v1, float v2, float v3);
  static void glUniformValuef(int8_t location, float v1, float v2, float v3, float v4);

  static void glUniformMatrix4x4Value(int8_t location, const float *values);

  static uint32_t glGetCurrentProgram();

  static int32_t glGetProgramiv(uint32_t program, glConst paramName);

  // Textures support
  static void glActiveTexture(glConst texBlock);
  static uint32_t glGenTexture();
  static void glBindTexture(uint32_t textureID);
  static void glTexImage2D(int width, int height, glConst layout, glConst pixelType, const void * data);
  static void glTexSubImage2D(int x, int y, int width, int height, glConst layout, glConst pixelType, const void * data);
  static void glTexParameter(glConst param, glConst value);

  // Draw support
  static void glDrawElements(uint16_t indexCount);
};

void CheckGLError();

#define GLCHECK(x) do { (x); CheckGLError(); } while (false)
#define GLCHECKCALL() do { CheckGLError(); } while (false)
