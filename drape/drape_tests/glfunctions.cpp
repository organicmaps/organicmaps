#include "drape/glfunctions.hpp"
#include "drape/drape_tests/glmock_functions.hpp"

#include "base/assert.hpp"

using namespace emul;

#define MOCK_CALL(f) GLMockFunctions::Instance().f;

void GLFunctions::glFlush()
{

}

uint32_t GLFunctions::glGenVertexArray()
{
  return MOCK_CALL(glGenVertexArray());
}

void GLFunctions::glBindVertexArray(uint32_t vao)
{
  MOCK_CALL(glBindVertexArray(vao));
}

void GLFunctions::glDeleteVertexArray(uint32_t vao)
{
  MOCK_CALL(glDeleteVertexArray(vao));
}

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

void GLFunctions::glBufferData(glConst target, uint32_t size, void const * data, glConst usage)
{
  MOCK_CALL(glBufferData(target, size, data, usage));
}

void GLFunctions::glBufferSubData(glConst target, uint32_t size, void const * data, uint32_t offset)
{
  MOCK_CALL(glBufferSubData(target, size, data, offset));
}

uint32_t GLFunctions::glCreateShader(glConst type)
{
  return MOCK_CALL(glCreateShader(type));
}

void GLFunctions::glShaderSource(uint32_t shaderID, string const & src)
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

int8_t GLFunctions::glGetAttribLocation(uint32_t programID, string const & name)
{
  return MOCK_CALL(glGetAttribLocation(programID, name));
}

void GLFunctions::glBindAttribLocation(uint32_t programID, uint8_t index, string const & name)
{

}

/// enable vertex attribute binding. To get attributeLocation need to call glGetAttributeLocation
void GLFunctions::glEnableVertexAttribute(int32_t attributeLocation)
{
  MOCK_CALL(glEnableVertexAttribute(attributeLocation));
}

void GLFunctions::glVertexAttributePointer(int32_t attrLocation,
                                           uint32_t count,
                                           glConst type,
                                           bool needNormalize,
                                           uint32_t stride,
                                           uint32_t offset)
{
  MOCK_CALL(glVertexAttributePointer(attrLocation, count, type, needNormalize, stride, offset));
}

int8_t GLFunctions::glGetUniformLocation(uint32_t programID, string const & name)
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

void GLFunctions::glUniformMatrix4x4Value(int8_t location, float const * values)
{
  MOCK_CALL(glUniformMatrix4x4Value(location, values));
}

uint32_t GLFunctions::glGetCurrentProgram()
{
  return MOCK_CALL(glGetCurrentProgram());
}

bool GLFunctions::glHasExtension(string const & extName)
{
  return MOCK_CALL(glHasExtension(extName));
}

int32_t GLFunctions::glGetProgramiv(uint32_t program, glConst paramName)
{
 return MOCK_CALL(glGetProgramiv(program, paramName));
}

void GLFunctions::glGetActiveUniform(uint32_t programID, uint32_t uniformIndex,
                                     int32_t * uniformSize, glConst * type, string &name)
{
  MOCK_CALL(glGetActiveUniform(programID, uniformIndex, uniformSize, type, name));
}

void GLFunctions::glActiveTexture(glConst texBlock)
{
  MOCK_CALL(glActiveTexture(texBlock));
}

uint32_t GLFunctions::glGenTexture()
{
  return MOCK_CALL(glGenTexture());
}

void GLFunctions::glDeleteTexture(uint32_t id)
{
  MOCK_CALL(glDeleteTexture(id));
}

void GLFunctions::glBindTexture(uint32_t textureID)
{
  MOCK_CALL(glBindTexture(textureID));
}

void GLFunctions::glTexImage2D(int width, int height, glConst layout, glConst pixelType, void const * data)
{
  MOCK_CALL(glTexImage2D(width, height, layout, pixelType, data));
}

void GLFunctions::glTexSubImage2D(int x, int y, int width, int height, glConst layout, glConst pixelType, void const * data)
{
  MOCK_CALL(glTexSubImage2D(x, y, width, height, layout, pixelType, data));
}

void GLFunctions::glTexParameter(glConst param, glConst value)
{
  MOCK_CALL(glTexParameter(param, value));
}

int32_t GLFunctions::glGetInteger(glConst pname)
{
  return MOCK_CALL(glGetInteger(pname));
}

void CheckGLError(my::SrcPoint const & /*srcPt*/) {}

// @TODO add actual unit tests
void GLFunctions::glEnable(glConst mode) {}

void GLFunctions::glBlendEquation(glConst function) {}

void GLFunctions::glBlendFunc(glConst srcFactor, glConst dstFactor) {}

void GLFunctions::glDisable(glConst mode) {}

void GLFunctions::glUniformValueiv(int8_t location, int32_t * v, uint32_t size) {}

void * GLFunctions::glMapBuffer(glConst target) { return 0; }

void GLFunctions::glUnmapBuffer(glConst target) {}

void GLFunctions::glDrawElements(uint16_t indexCount) {}

void GLFunctions::glPixelStore(glConst name, uint32_t value) {}
