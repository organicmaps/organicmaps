#pragma once

#include "../std/string.hpp"
#include "../std/stdint.hpp"

class Shader
{
public:
  enum Type
  {
    VertexShader,
    FragmentShader
  };

  Shader(string const & shaderSource, Type type);
  ~Shader();

  int GetID() const;

private:
  string const m_source;
  Type m_type;
  uint32_t m_glID;
};
