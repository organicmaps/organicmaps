#pragma once

#include "shaders/program_params.hpp"
#include "shaders/program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/gpu_program.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "base/macros.hpp"
#include "base/thread_checker.hpp"

#include <array>
#include <string>

namespace gpu
{
class ProgramManager
{
public:
  ProgramManager() = default;

  void Init(ref_ptr<dp::GraphicsContext> context);
  void Destroy(ref_ptr<dp::GraphicsContext> context);

  ref_ptr<dp::GpuProgram> GetProgram(Program program);
  ref_ptr<ProgramParamsSetter> GetParamsSetter() const;

private:
  void InitForOpenGL(ref_ptr<dp::GraphicsContext> context);
  void InitForVulkan(ref_ptr<dp::GraphicsContext> context);
  void DestroyForVulkan(ref_ptr<dp::GraphicsContext> context);

#if defined(OMIM_METAL_AVAILABLE)
  // Definition of this method is in a .mm-file.
  void InitForMetal(ref_ptr<dp::GraphicsContext> context);
  void DestroyForMetal(ref_ptr<dp::GraphicsContext> context);
#endif

  using Programs = std::array<drape_ptr<dp::GpuProgram>, static_cast<size_t>(Program::ProgramsCount)>;
  drape_ptr<ProgramPool> m_pool;
  Programs m_programs;
  drape_ptr<ProgramParamsSetter> m_paramsSetter;

  DECLARE_THREAD_CHECKER(m_threadChecker);
  DISALLOW_COPY_AND_MOVE(ProgramManager);
};
}  // namespace gpu
