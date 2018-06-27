#pragma once

#include "shaders/programs.hpp"

#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"

#include <string>

namespace gpu
{
class ProgramPool
{
public:
  virtual ~ProgramPool() = default;
  virtual drape_ptr<dp::GpuProgram> Get(Program program) = 0;
};
}  // namespace gpu
