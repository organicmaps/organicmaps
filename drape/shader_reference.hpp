#pragma once

#include "../std/string.hpp"

class ShaderReference
{
public:
  enum Type
  {
    VertexShader,
    FragmentShader
  };

  ShaderReference(const string & shaderSource, Type type);

  int GetID();
  void Ref();
  void Deref();

private:
  const string m_source;
  Type m_type;
  uint32_t m_glID;
  int32_t m_refCount;
};
