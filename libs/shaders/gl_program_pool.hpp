#pragma once

#include "shaders/program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"
#include "drape/shader.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace gpu
{
class GLProgramPool : public ProgramPool
{
public:
  explicit GLProgramPool(dp::ApiVersion apiVersion, std::string_view additionalDefines = {});
  ~GLProgramPool() override;

  drape_ptr<dp::GpuProgram> Get(Program program) override;

private:
  ref_ptr<dp::Shader> GetShader(std::string const & name, std::string const & source, dp::Shader::Type type);

  dp::ApiVersion const m_apiVersion;

  std::map<std::string, drape_ptr<dp::Shader>> m_shaders;
  std::string m_defines;
};
}  // namespace gpu
