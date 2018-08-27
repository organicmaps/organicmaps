#include "shaders/program_manager.hpp"
#include "shaders/metal_program_params.hpp"
#include "shaders/metal_program_pool.hpp"

#include "drape/metal/metal_base_context.hpp"

namespace gpu
{
void ProgramManager::InitForMetal(ref_ptr<dp::GraphicsContext> context)
{
  ASSERT(dynamic_cast<dp::metal::MetalBaseContext *>(context.get()) != nullptr, ());
  ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
  m_pool = make_unique_dp<metal::MetalProgramPool>(metalContext->GetMetalDevice());
  m_paramsSetter = make_unique_dp<metal::MetalProgramParamsSetter>();
}
}  // namespace gpu
