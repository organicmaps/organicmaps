#pragma once

#include <cstdint>
#include <string>

namespace dp
{
class Shader
{
public:
  enum class Type
  {
    VertexShader,
    FragmentShader
  };

  Shader(std::string const & shaderName, std::string const & shaderSource, std::string const & defines, Type type);
  ~Shader();

  uint32_t GetID() const;

private:
  std::string const m_shaderName;
  uint32_t m_glID;
};
}  // namespace dp
