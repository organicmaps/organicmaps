#pragma once

#include "drape/glfunctions.hpp"

#include "std/string.hpp"
#include "std/cstdint.hpp"

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

  Shader(string const & shaderSource, string const & defines, Type type);
  ~Shader();

  int GetID() const;

private:
  uint32_t m_glID;
};

void PreprocessShaderSource(string & src);

} // namespace dp
