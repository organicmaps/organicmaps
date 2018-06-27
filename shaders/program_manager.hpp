#pragma once

#include "shaders/program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"

#include "base/macros.hpp"

#include <array>
#include <string>

namespace gpu
{
class ProgramManager
{
public:
  ProgramManager() = default;

  void Init(dp::ApiVersion apiVersion);

  ref_ptr<dp::GpuProgram> GetProgram(Program program);

private:
  using Programs = std::array<drape_ptr<dp::GpuProgram>,
                              static_cast<size_t>(Program::ProgramsCount)>;
  Programs m_programs;
  drape_ptr<ProgramPool> m_pool;

  DISALLOW_COPY_AND_MOVE(ProgramManager);
};
}  // namespace gpu
