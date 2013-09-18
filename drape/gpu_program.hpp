#pragma once

#include "shader_reference.hpp"
#include "pointers.hpp"

#include "../std/string.hpp"

class GpuProgram
{
public:
  GpuProgram(ReferencePoiner<ShaderReference> vertexShader,
             ReferencePoiner<ShaderReference> fragmentShader);
  ~GpuProgram();

  void Bind();
  void Unbind();

  int8_t GetAttributeLocation(const string & attributeName) const;
  int8_t GetUniformLocation(const string & uniformName) const;
  void ActivateSampler(uint8_t textureBlock, const string & samplerName);

private:
  uint32_t m_programID;
};
