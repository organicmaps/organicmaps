#pragma once

#include "shaders/program_pool.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <array>

namespace gpu
{
namespace vulkan
{
class VulkanProgramPool : public ProgramPool
{
public:
  explicit VulkanProgramPool(ref_ptr<dp::GraphicsContext> context);
  ~VulkanProgramPool() override;

  void Destroy(ref_ptr<dp::GraphicsContext> context);
  drape_ptr<dp::GpuProgram> Get(Program program) override;

private:
  std::array<drape_ptr<dp::vulkan::VulkanGpuProgram>,
             static_cast<size_t>(Program::ProgramsCount)> m_programs;
};
}  // namespace vulkan
}  // namespace gpu
