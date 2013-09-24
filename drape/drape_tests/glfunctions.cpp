#include "../glfunctions.hpp"
#include "glmock_functions.hpp"

#include "../../base/assert.hpp"

using namespace emul;

#define MOCK_CALL(f) GLMockFunctions::Instance().f;

uint32_t GLFunctions::glGenBuffer()
{
  return MOCK_CALL(glGenBuffer());
}

void GLFunctions::glBindBuffer(uint32_t vbo, glConst target)
{
  MOCK_CALL(glBindBuffer(vbo, target));
}

void GLFunctions::glDeleteBuffer(uint32_t vbo)
{
  MOCK_CALL(glDeleteBuffer(vbo));
}

void GLFunctions::glBufferData(glConst target, uint32_t size, const void * data, glConst usage)
{
  MOCK_CALL(glBufferData(target, size, data, usage));
}

void GLFunctions::glBufferSubData(glConst target, uint32_t size, const void *data, uint32_t offset)
{
  MOCK_CALL(glBufferSubData(target, size, data, offset));
}

uint32_t GLFunctions::glCreateShader(glConst type)
{
  return MOCK_CALL(glCreateShader(type));
}

void GLFunctions::glShaderSource(uint32_t shaderID, const string & src)
{
  MOCK_CALL(glShaderSource(shaderID, src));
}

bool GLFunctions::glCompileShader(uint32_t shaderID, string & errorLog)
{
  return MOCK_CALL(glCompileShader(shaderID, errorLog));
}

void GLFunctions::glDeleteShader(uint32_t shaderID)
{
  MOCK_CALL(glDeleteShader(shaderID));
}

uint32_t GLFunctions::glCreateProgram()
{
  return MOCK_CALL(glCreateProgram());
}

void GLFunctions::glAttachShader(uint32_t programID, uint32_t shaderID)
{
  MOCK_CALL(glAttachShader(programID, shaderID));
}

void GLFunctions::glDetachShader(uint32_t programID, uint32_t shaderID)
{
  MOCK_CALL(glDetachShader(programID, shaderID));
}

bool GLFunctions::glLinkProgram(uint32_t programID, string & errorLog)
{
  return MOCK_CALL(glLinkProgram(programID, errorLog));
}

void GLFunctions::glDeleteProgram(uint32_t programID)
{
  MOCK_CALL(glDeleteProgram(programID));
}

void GLFunctions::glUseProgram(uint32_t programID)
{
  MOCK_CALL(glUseProgram(programID));
}

int8_t GLFunctions::glGetAttribLocation(uint32_t programID, const string & name)
{
  return 0;
}

void GLFunctions::glBindAttribLocation(uint32_t programID, uint8_t index, const string & name)
{

}

/// enable vertex attribute binding. To get attributeLocation need to call glGetAttributeLocation
void GLFunctions::glEnableVertexAttribute(int32_t attributeLocation)
{

}

void GLFunctions::glVertexAttributePointer(int32_t attrLocation,
                                           uint32_t count,
                                           glConst type,
                                           bool needNormalize,
                                           uint32_t stride,
                                           uint32_t offset)
{

}

int8_t GLFunctions::glGetUniformLocation(uint32_t programID, const string & name)
{
  return MOCK_CALL(glGetUniformLocation(programID, name));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v)
{
  MOCK_CALL(glUniformValuei(location, v));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2)
{
  MOCK_CALL(glUniformValuei(location, v1, v2));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3)
{
  MOCK_CALL(glUniformValuei(location, v1, v2, v3));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
  MOCK_CALL(glUniformValuei(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformValuef(int8_t location, float v)
{
  MOCK_CALL(glUniformValuef(location, v));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2)
{
  MOCK_CALL(glUniformValuef(location, v1, v2));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3)
{
  MOCK_CALL(glUniformValuef(location, v1, v2, v3));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3, float v4)
{
  MOCK_CALL(glUniformValuef(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformMatrix4x4Value(int8_t location, float * values)
{
  MOCK_CALL(glUniformMatrix4x4Value(location, values));
}

static uint32_t glGetCurrentProgram()
{
  return MOCK_CALL(glGetCurrentProgram());
}

void CheckGLError() {}
