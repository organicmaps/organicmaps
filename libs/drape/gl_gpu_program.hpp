#pragma once

#include "drape/gl_constants.hpp"
#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"
#include "drape/shader.hpp"

#include <map>
#include <string>
#include <vector>

namespace dp
{
class GLGpuProgram : public GpuProgram
{
public:
  GLGpuProgram(std::string const & programName, ref_ptr<Shader> vertexShader, ref_ptr<Shader> fragmentShader);
  ~GLGpuProgram() override;

  void Bind() override;
  void Unbind() override;

  int8_t GetAttributeLocation(std::string const & attributeName) const;
  int8_t GetUniformLocation(std::string const & uniformName) const;
  glConst GetUniformType(std::string const & uniformName) const;

  struct UniformInfo
  {
    int8_t m_location = -1;
    glConst m_type = gl_const::GLFloatType;
  };

  using UniformsInfo = std::map<std::string, UniformInfo>;
  UniformsInfo const & GetUniformsInfo() const;
  uint32_t GetNumericUniformsCount() const { return m_numericUniformsCount; }

private:
  void LoadUniformLocations();
  uint32_t CalculateNumericUniformsCount() const;

  uint32_t m_programID;

  ref_ptr<Shader> m_vertexShader;
  ref_ptr<Shader> m_fragmentShader;

  UniformsInfo m_uniforms;
  uint8_t m_textureSlotsCount = 0;
  uint32_t m_numericUniformsCount = 0;
};
}  // namespace dp
