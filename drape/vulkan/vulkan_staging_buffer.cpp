#include "drape/vulkan/vulkan_staging_buffer.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace dp
{
namespace vulkan
{
VulkanStagingBuffer::VulkanStagingBuffer(ref_ptr<VulkanObjectManager> objectManager,
                                         uint32_t sizeInBytes)
  : m_objectManager(objectManager)
  , m_sizeInBytes(sizeInBytes)
{
  auto constexpr kStagingBuffer = VulkanMemoryManager::ResourceType::Staging;
  VkDevice device = m_objectManager->GetDevice();
  auto const & mm = m_objectManager->GetMemoryManager();

  m_object = m_objectManager->CreateBuffer(kStagingBuffer, sizeInBytes, 0 /* batcherHash */);
  VkMemoryRequirements memReqs = {};
  vkGetBufferMemoryRequirements(device, m_object.m_buffer, &memReqs);
  m_sizeAlignment = mm.GetSizeAlignment(memReqs);
  m_offsetAlignment = mm.GetOffsetAlignment(kStagingBuffer);

  CHECK_VK_CALL(vkBindBufferMemory(device, m_object.m_buffer, m_object.GetMemory(),
                                   m_object.GetAlignedOffset()));

  m_pointer = m_objectManager->Map(m_object);
}

VulkanStagingBuffer::~VulkanStagingBuffer()
{
  m_objectManager->Unmap(m_object);
  m_objectManager->DestroyObject(m_object);
}

bool VulkanStagingBuffer::HasEnoughSpace(uint32_t sizeInBytes) const
{
  auto const & mm = m_objectManager->GetMemoryManager();
  auto const alignedSize = mm.GetAligned(sizeInBytes, m_sizeAlignment);
  return m_offset + alignedSize <= m_sizeInBytes;
}

VulkanStagingBuffer::StagingData VulkanStagingBuffer::Reserve(uint32_t sizeInBytes)
{
  CHECK(m_offset % m_offsetAlignment == 0, ());
  CHECK(HasEnoughSpace(sizeInBytes), ());
  auto const & mm = m_objectManager->GetMemoryManager();

  auto const alignedOffset = m_offset;
  auto const alignedSize = mm.GetAligned(sizeInBytes, m_sizeAlignment);
  uint8_t * ptr = m_pointer + alignedOffset;

  // Update offset and align it.
  m_offset += alignedSize;
  m_offset = std::min(mm.GetAligned(m_offset, m_offsetAlignment), m_object.GetAlignedSize());

  StagingData result;
  result.m_stagingBuffer = m_object.m_buffer;
  result.m_pointer = ptr;
  result.m_offset = alignedOffset;
  result.m_size = sizeInBytes;
  return result;
}

uint32_t VulkanStagingBuffer::ReserveWithId(uint32_t sizeInBytes, StagingData & data)
{
  data = Reserve(sizeInBytes);
  m_reservation.push_back(data);
  return static_cast<uint32_t>(m_reservation.size()) - 1;
}

VulkanStagingBuffer::StagingData const & VulkanStagingBuffer::GetReservationById(uint32_t id) const
{
  CHECK_LESS(id, m_reservation.size(), ());
  return m_reservation[id];
}

void VulkanStagingBuffer::Flush()
{
  if (m_offset == 0)
    return;

  auto const size = m_offset;
  m_objectManager->Flush(m_object, 0 /* offset */, size);
}

void VulkanStagingBuffer::Reset()
{
  m_offset = 0;
  m_reservation.clear();
}
}  // namespace vulkan
}  // namespace dp
