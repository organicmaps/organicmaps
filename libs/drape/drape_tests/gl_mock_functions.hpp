#pragma once

#include "drape/drape_global.hpp"
#include "drape/gl_constants.hpp"

#include <gmock/gmock.h>

#include <string>

namespace emul
{
class GLMockFunctions
{
public:
  static void Init(int * argc, char ** argv);
  static void Teardown();
  static GLMockFunctions & Instance();
  static void ValidateAndClear();

  MOCK_METHOD1(Init, void(dp::ApiVersion));

  // VAO
  MOCK_METHOD0(glGenVertexArray, uint32_t());
  MOCK_METHOD1(glBindVertexArray, void(uint32_t vao));
  MOCK_METHOD1(glDeleteVertexArray, void(uint32_t vao));

  // VBO
  MOCK_METHOD0(glGenBuffer, uint32_t());
  MOCK_METHOD2(glBindBuffer, void(uint32_t vbo, glConst target));
  MOCK_METHOD1(glDeleteBuffer, void(uint32_t vbo));
  MOCK_METHOD4(glBufferData, void(glConst target, uint32_t size, void const * data, glConst usage));
  MOCK_METHOD4(glBufferSubData, void(glConst target, uint32_t size, void const * data, uint32_t offset));
  MOCK_METHOD2(glGetBufferParameter, int32_t(glConst target, glConst name));

  MOCK_METHOD2(glGetUniformLocation, int8_t(uint32_t programID, std::string const & name));
  MOCK_METHOD2(glUniformValuei, void(int8_t location, int32_t v));
  MOCK_METHOD3(glUniformValuei, void(int8_t location, int32_t v1, int32_t v2));
  MOCK_METHOD4(glUniformValuei, void(int8_t location, int32_t v1, int32_t v2, int32_t v3));
  MOCK_METHOD5(glUniformValuei, void(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4));

  MOCK_METHOD2(glUniformValuef, void(int8_t location, float v));
  MOCK_METHOD3(glUniformValuef, void(int8_t location, float v1, float v2));
  MOCK_METHOD4(glUniformValuef, void(int8_t location, float v1, float v2, float v3));
  MOCK_METHOD5(glUniformValuef, void(int8_t location, float v1, float v2, float v3, float v4));
  MOCK_METHOD2(glUniformMatrix4x4Value, void(int8_t location, float const * values));
  MOCK_METHOD0(glGetCurrentProgram, uint32_t());

  MOCK_METHOD1(glCreateShader, uint32_t(glConst type));
  MOCK_METHOD2(glShaderSource, void(uint32_t shaderID, std::string const & src));
  MOCK_METHOD2(glCompileShader, bool(uint32_t shaderID, std::string & errorLog));
  MOCK_METHOD1(glDeleteShader, void(uint32_t shaderID));

  MOCK_METHOD0(glCreateProgram, uint32_t());
  MOCK_METHOD2(glAttachShader, void(uint32_t programID, uint32_t shaderID));
  MOCK_METHOD2(glDetachShader, void(uint32_t programID, uint32_t shaderID));
  MOCK_METHOD2(glLinkProgram, bool(uint32_t programID, std::string & errorLog));
  MOCK_METHOD1(glDeleteProgram, void(uint32_t programID));

  MOCK_METHOD2(glGetAttribLocation, int32_t(uint32_t programID, std::string const & name));
  MOCK_METHOD1(glEnableVertexAttribute, void(int32_t attributeLocation));
  MOCK_METHOD6(glVertexAttributePointer, void(int32_t attrLocation, uint32_t count, glConst type, bool needNormalize,
                                              uint32_t stride, uint32_t offset));

  MOCK_METHOD1(glUseProgram, void(uint32_t programID));
  MOCK_METHOD1(glHasExtension, bool(std::string const & extName));

  MOCK_METHOD2(glGetProgramiv, int32_t(uint32_t, glConst));

  MOCK_METHOD5(glGetActiveUniform, void(uint32_t, uint32_t, int32_t *, glConst *, std::string &));

  // Texture functions
  MOCK_METHOD1(glActiveTexture, void(glConst));
  MOCK_METHOD0(glGenTexture, uint32_t());
  MOCK_METHOD1(glDeleteTexture, void(uint32_t));
  MOCK_METHOD1(glBindTexture, void(uint32_t));
  MOCK_METHOD5(glTexImage2D, void(int, int, glConst, glConst, void const *));
  MOCK_METHOD7(glTexSubImage2D, void(int, int, int, int, glConst, glConst, void const *));
  MOCK_METHOD2(glTexParameter, void(glConst, glConst));

  MOCK_METHOD1(glGetInteger, int32_t(glConst));
  MOCK_METHOD1(glGetString, std::string(glConst));
  MOCK_METHOD0(glGetMaxLineWidth, int32_t());

  MOCK_METHOD1(glLineWidth, void(uint32_t value));
  MOCK_METHOD4(glViewport, void(uint32_t x, uint32_t y, uint32_t w, uint32_t h));
  MOCK_METHOD4(glScissor, void(uint32_t x, uint32_t y, uint32_t w, uint32_t h));

  // FBO
  MOCK_METHOD1(glGenFramebuffer, void(uint32_t * fbo));
  MOCK_METHOD1(glBindFramebuffer, void(uint32_t fbo));
  MOCK_METHOD1(glDeleteFramebuffer, void(uint32_t * fbo));
  MOCK_METHOD2(glFramebufferTexture2D, void(glConst attachment, glConst texture));
  MOCK_METHOD0(glCheckFramebufferStatus, uint32_t());

  MOCK_METHOD1(glCullFace, void(glConst));
  MOCK_METHOD1(glFrontFace, void(glConst));

  MOCK_METHOD1(glDepthMask, void(bool));

  MOCK_METHOD1(glClear, void(uint32_t));
  MOCK_METHOD4(glClearColor, void(float, float, float, float));
  MOCK_METHOD1(glClearDepthValue, void(double));

private:
  static GLMockFunctions * m_mock;
};
}  // namespace emul

#define EXPECTGL(x) EXPECT_CALL(emul::GLMockFunctions::Instance(), x)
