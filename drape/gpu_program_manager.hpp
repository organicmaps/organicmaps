#pragma once

#include "drape/pointers.hpp"
#include "drape/gpu_program.hpp"
#include "drape/gpu_program_info.hpp"
#include "drape/shader.hpp"

#include "base/macros.hpp"

#include <map>
#include <string>

namespace dp
{
class GpuProgramManager
{
public:
  GpuProgramManager() = default;
  ~GpuProgramManager();

  void Init(drape_ptr<gpu::GpuProgramGetter> && programGetter);

  ref_ptr<GpuProgram> GetProgram(int index);

private:
  ref_ptr<Shader> GetShader(int index, std::string const & source, Shader::Type t);

  using ProgramMap = std::map<int, drape_ptr<GpuProgram>>;
  using ShaderMap = std::map<int, drape_ptr<Shader>>;
  ProgramMap m_programs;
  ShaderMap m_shaders;
  std::string m_globalDefines;
  uint8_t m_minTextureSlotsCount = 0;
  drape_ptr<gpu::GpuProgramGetter> m_programGetter;

  DISALLOW_COPY_AND_MOVE(GpuProgramManager);
};
}  // namespace dp
