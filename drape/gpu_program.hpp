#pragma once

#include "shader.hpp"
#include "pointers.hpp"

#include "../std/string.hpp"

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
};
