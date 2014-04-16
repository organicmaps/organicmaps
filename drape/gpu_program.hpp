#pragma once

#include "shader.hpp"
#include "pointers.hpp"
#include "glconstants.hpp"

#include "../std/string.hpp"

#ifdef DEBUG
  #include "../std/scoped_ptr.hpp"

  class UniformValidator;
  typedef int32_t UniformSize;
  typedef pair<glConst, UniformSize> UniformTypeAndSize;
#endif

class GpuProgram
{
public:
  GpuProgram(RefPointer<Shader> vertexShader,
             RefPointer<Shader> fragmentShader);
  ~GpuProgram();

  void Bind();
  void Unbind();

  int8_t GetAttributeLocation(string const & attributeName) const;
  int8_t GetUniformLocation(string const & uniformName) const;

private:
  uint32_t m_programID;

#ifdef DEBUG
private:
  scoped_ptr<UniformValidator> m_validator;
public:
  bool HasUniform(string const & name, glConst type, UniformSize size);
#endif
};
