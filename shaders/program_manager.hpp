#pragma once

#include "shaders/program_pool.hpp"
#include "shaders/program_params.hpp"

#include "drape/drape_global.hpp"
#include "drape/graphics_context.hpp"
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

  void Init(ref_ptr<dp::GraphicsContext> context);

  ref_ptr<dp::GpuProgram> GetProgram(Program program);
  ref_ptr<ProgramParamsSetter> GetParamsSetter() const;

private:
  void InitForOpenGL(ref_ptr<dp::GraphicsContext> context);
  // Definition of this method is in a .mm-file.
  void InitForMetal(ref_ptr<dp::GraphicsContext> context);
  
  using Programs = std::array<drape_ptr<dp::GpuProgram>,
                              static_cast<size_t>(Program::ProgramsCount)>;
  Programs m_programs;
  drape_ptr<ProgramPool> m_pool;
  drape_ptr<ProgramParamsSetter> m_paramsSetter;

  DISALLOW_COPY_AND_MOVE(ProgramManager);
};
}  // namespace gpu
