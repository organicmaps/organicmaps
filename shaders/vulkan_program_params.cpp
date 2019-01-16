#include "shaders/vulkan_program_params.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"

namespace gpu
{
namespace vulkan
{
namespace
{
template<typename T>
void ApplyBytes(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                T const & params)
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  ref_ptr<dp::vulkan::VulkanGpuProgram> p = program;
  //TODO(@rokuz, @darina): Implement.
  CHECK(false, ());
}
}  // namespace
  
void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      MapProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      RouteProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      TrafficProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      TransitProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      GuiProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      ShapesProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      Arrow3dProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      DebugRectProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      ScreenQuadProgramParams const & params)
{
  ApplyBytes(context, program, params);
}

void VulkanProgramParamsSetter::Apply(ref_ptr<dp::GraphicsContext> context,
                                      ref_ptr<dp::GpuProgram> program,
                                      SMAAProgramParams const & params)
{
  ApplyBytes(context, program, params);
}
}  // namespace vulkan
}  // namespace gpu
