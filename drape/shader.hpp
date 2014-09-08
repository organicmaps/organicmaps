#pragma once

#include "glfunctions.hpp"

#include "../std/string.hpp"
#include "../std/stdint.hpp"

namespace dp
{

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
  string m_source;
  Type m_type;
  uint32_t m_glID;
};

void PreprocessShaderSource(string & src);

} // namespace dp
