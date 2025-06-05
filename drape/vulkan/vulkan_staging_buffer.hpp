#pragma once

#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanStagingBuffer
{
public:
  VulkanStagingBuffer(ref_ptr<VulkanObjectManager> objectManager, uint32_t sizeInBytes);
  ~VulkanStagingBuffer();

  struct StagingData
  {
    VkBuffer m_stagingBuffer = {};
    uint8_t * m_pointer = nullptr;
    uint32_t m_offset = 0;
    uint32_t m_size = 0;
    operator bool() const { return m_stagingBuffer != 0 && m_pointer != nullptr; }
  };

  bool HasEnoughSpace(uint32_t sizeInBytes) const;

  StagingData Reserve(uint32_t sizeInBytes);
  uint32_t ReserveWithId(uint32_t sizeInBytes, StagingData & data);
  StagingData const & GetReservationById(uint32_t id) const;
  void Flush();
  void Reset();

private:
  ref_ptr<VulkanObjectManager> m_objectManager;
  uint32_t m_sizeInBytes;
  VulkanObject m_object;
  uint32_t m_offsetAlignment = 0;
  uint32_t m_sizeAlignment = 0;
  uint8_t * m_pointer = nullptr;
  uint32_t m_offset = 0;
  std::vector<StagingData> m_reservation;
  ThreadChecker m_threadChecker;
};
}  // namespace vulkan
}  // namespace dp
