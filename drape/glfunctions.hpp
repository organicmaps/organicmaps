#pragma once

#include "../std/string.hpp"

typedef uint32_t glConst;

namespace GLConst
{
  /// Buffer targets
  extern const glConst GLArrayBuffer;
  extern const glConst GLElementArrayBuffer;

  /// BufferUsage
  extern const glConst GLStaticDraw;
  extern const glConst GLStreamDraw;
  extern const glConst GLDynamicDraw;

  /// ShaderType
  extern const glConst GLVertexShader;
  extern const glConst GLFragmentShader;
  extern const glConst GLCurrentProgram;

  /// Pixel type for texture upload
  extern const glConst GL8BitOnChannel;
  extern const glConst GL4BitOnChannel;

  /// OpenGL types
  extern const glConst GLByteType;
  extern const glConst GLUnsignedByteType;
  extern const glConst GLShortType;
  extern const glConst GLUnsignedShortType;
  extern const glConst GLIntType;
  extern const glConst GLUnsignedIntType;
  extern const glConst GLFloatType;
  extern const glConst GLDoubleType;
}

class GLFunctions
{
public:
  static void Init();

  static bool glHasExtension(const string & name);

  /// VAO support
  static int glGenVertexArray();
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

  static int8_t glGetUniformLocation(uint32_t programID, const string & name);
  static void glUniformValue(int8_t location, int32_t v);
  static void glUniformValue(int8_t location, int32_t v1, int32_t v2);
  static void glUniformValue(int8_t location, int32_t v1, int32_t v2, int32_t v3);
  static void glUniformValue(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  static void glUniformValue(int8_t location, float v);
  static void glUniformValue(int8_t location, float v1, float v2);
  static void glUniformValue(int8_t location, float v1, float v2, float v3);
  static void glUniformValue(int8_t location, float v1, float v2, float v3, float v4);

  static void glUniformMatrix4x4Value(int8_t location, float * values);

  static uint32_t glGetCurrentProgram();

  // Textures support
  static void glActiveTexture(uint32_t samplerBlock);
  static uint32_t glGenTexture();
  static void glBindTexture(uint32_t textureID);
  static void glTexImage2D(int width, int height, glConst pixelType, const void * data);
  static void glTexSubImage2D(int x, int y, int width, int height, glConst pixelType, const void * data);
  static uint32_t  glGetBindedTexture();

  // Draw support
  static void glDrawElements(uint16_t indexCount);
};
