#pragma once

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <memory>

namespace dp
{
namespace vulkan
{
struct DeviceHolder
{
  VkDevice const m_device;
  explicit DeviceHolder(VkDevice const device) : m_device(device) {}
};

using DeviceHolderPtr = std::weak_ptr<DeviceHolder>;
}  // namespace vulkan
}  // namespace dp
