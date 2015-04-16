#pragma once

#include "drape/shader.hpp"
#include "drape/pointers.hpp"
#include "drape/glconstants.hpp"

#include "std/string.hpp"

#ifdef DEBUG
  #include "std/unique_ptr.hpp"
#endif

namespace dp
{

#ifdef DEBUG
  class UniformValidator;
  typedef int32_t UniformSize;
  typedef pair<glConst, UniformSize> UniformTypeAndSize;
#endif

class GpuProgram
{
public:
  GpuProgram(ref_ptr<Shader> vertexShader,
             ref_ptr<Shader> fragmentShader);
  ~GpuProgram();

  void Bind();
  void Unbind();

  int8_t GetAttributeLocation(string const & attributeName) const;
  int8_t GetUniformLocation(string const & uniformName) const;

private:
  uint32_t m_programID;

#ifdef DEBUG
private:
  unique_ptr<UniformValidator> m_validator;
public:
  bool HasUniform(string const & name, glConst type, UniformSize size);
#endif
};

} // namespace dp
