#pragma once

#include "drape/mesh_object.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_device_holder.hpp"

#include "base/assert.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanMeshObjectImpl : public MeshObjectImpl
{
public:
  VulkanMeshObjectImpl(ref_ptr<dp::MeshObject> mesh)
    : m_mesh(std::move(mesh))
  {}
  
  void Build(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program) override;
  void Reset() override;
  
  void UpdateBuffer(uint32_t bufferInd) override;
  void Bind(ref_ptr<dp::GpuProgram> program) override {}
  void Unbind() override {}
  
  void DrawPrimitives(ref_ptr<dp::GraphicsContext> context, uint32_t verticesCount) override;
  
private:
  ref_ptr<dp::MeshObject> m_mesh;
  DeviceHolderPtr m_deviceHolder;
  std::vector<VkBuffer> m_geometryBuffers;
};
}  // namespace vulkan
}  // namespace dp
