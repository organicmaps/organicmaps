#include "drape/vulkan/vulkan_staging_buffer.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace dp
{
namespace vulkan
{
// The most GPUs use this value, real one can be known only after buffer creation.
uint32_t constexpr kDefaultAlignment = 64;

VulkanStagingBuffer::VulkanStagingBuffer(ref_ptr<VulkanObjectManager> objectManager, uint32_t sizeInBytes)
  : m_objectManager(objectManager)
  , m_sizeInBytes(VulkanMemoryManager::GetAligned(sizeInBytes, kDefaultAlignment))
{
  auto constexpr kStagingBuffer = VulkanMemoryManager::ResourceType::Staging;
  VkDevice device = m_objectManager->GetDevice();
  auto const & mm = m_objectManager->GetMemoryManager();

  m_object = m_objectManager->CreateBuffer(kStagingBuffer, m_sizeInBytes, 0 /* batcherHash */);
  VkMemoryRequirements memReqs = {};
  vkGetBufferMemoryRequirements(device, m_object.m_buffer, &memReqs);

  // We must be able to map the whole range.
  m_sizeAlignment = mm.GetSizeAlignment(memReqs);
  if (!HasEnoughSpace(m_sizeInBytes))
  {
    // This GPU uses non-standard alignment we have to recreate buffer.
    auto const originalSize = m_sizeInBytes;
    auto const originalAlignment = m_sizeAlignment;
    m_sizeInBytes = VulkanMemoryManager::GetAligned(sizeInBytes, m_sizeAlignment);
    m_objectManager->DestroyObjectUnsafe(m_object);
    m_object = m_objectManager->CreateBuffer(kStagingBuffer, m_sizeInBytes, 0 /* batcherHash */);
    vkGetBufferMemoryRequirements(device, m_object.m_buffer, &memReqs);
    m_sizeAlignment = mm.GetSizeAlignment(memReqs);
    CHECK(HasEnoughSpace(m_sizeInBytes), ("originalSize =", originalSize, "originalAlignment =", originalAlignment,
                                          "m_sizeInBytes =", m_sizeInBytes, "m_sizeAlignment =", m_sizeAlignment));
  }

  m_offsetAlignment = mm.GetOffsetAlignment(kStagingBuffer);
  m_pointer = m_objectManager->MapUnsafe(m_object);
}

VulkanStagingBuffer::~VulkanStagingBuffer()
{
  m_objectManager->UnmapUnsafe(m_object);
  m_objectManager->DestroyObject(m_object);
}

bool VulkanStagingBuffer::HasEnoughSpace(uint32_t sizeInBytes) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const & mm = m_objectManager->GetMemoryManager();
  auto const alignedSize = mm.GetAligned(sizeInBytes, m_sizeAlignment);
  return m_offset + alignedSize <= m_sizeInBytes;
}

VulkanStagingBuffer::StagingData VulkanStagingBuffer::Reserve(uint32_t sizeInBytes)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
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
  CHECK_THREAD_CHECKER(m_threadChecker, ());
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
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_offset == 0)
    return;

  auto const size = m_offset;
  m_objectManager->FlushUnsafe(m_object, 0 /* offset */, size);
}

void VulkanStagingBuffer::Reset()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_offset = 0;
  m_reservation.clear();
}
}  // namespace vulkan
}  // namespace dp
