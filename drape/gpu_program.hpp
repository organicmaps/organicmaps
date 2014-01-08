#pragma once

#include "shader.hpp"
#include "pointers.hpp"
#include "glconstants.hpp"

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"

class UniformValidator;

class GpuProgram
{
public:
  GpuProgram(RefPointer<Shader> vertexShader,
             RefPointer<Shader> fragmentShader);
  ~GpuProgram();

  void Bind();
  void Unbind();

  int8_t GetAttributeLocation(const string & attributeName) const;
  int8_t GetUniformLocation(const string & uniformName) const;
  void ActivateSampler(uint8_t textureBlock, const string & samplerName);

private:
  uint32_t m_programID;

  scoped_ptr<UniformValidator> m_validator;
};

class UniformValidator
{
private:
  typedef int32_t UniformSize;
  typedef pair<glConst, UniformSize> UniformTypeAndSize;

  uint32_t m_programID;
  map<string, UniformTypeAndSize> m_uniformsMap;

public:
  UniformValidator(uint32_t programId);

  bool HasUniform(string const & name);
  bool HasValidTypeAndSizeForName(string const & name, glConst type, UniformSize size);
};
