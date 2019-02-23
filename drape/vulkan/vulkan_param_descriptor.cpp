#include "drape/vulkan/vulkan_param_descriptor.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace dp
{
namespace vulkan
{
void DescriptorSetGroup::Update(VkDevice device, std::vector<ParamDescriptor> const & descriptors)
{
  size_t const writeDescriptorsCount = descriptors.size();
  CHECK_LESS_OR_EQUAL(writeDescriptorsCount, kMaxDescriptorSets, ());

  std::array<uint32_t, kMaxDescriptorSets> ids = {};
  for (size_t i = 0; i < writeDescriptorsCount; ++i)
    ids[i] = descriptors[i].m_id;

  if (m_updated && ids == m_ids)
    return;

  m_ids = ids;
  m_updated = true;
  std::array<VkWriteDescriptorSet, kMaxDescriptorSets> writeDescriptorSets = {};
  for (size_t i = 0; i < writeDescriptorsCount; ++i)
  {
    writeDescriptorSets[i] = {};
    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = m_descriptorSet;
    writeDescriptorSets[i].descriptorCount = 1;
    if (descriptors[i].m_type == ParamDescriptor::Type::DynamicUniformBuffer)
    {
      writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      writeDescriptorSets[i].dstBinding = 0;
      writeDescriptorSets[i].pBufferInfo = &descriptors[i].m_bufferDescriptor;
    }
    else if (descriptors[i].m_type == ParamDescriptor::Type::Texture)
    {
      writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(descriptors[i].m_textureSlot);
      writeDescriptorSets[i].pImageInfo = &descriptors[i].m_imageDescriptor;
    }
    else
    {
      CHECK(false, ("Unsupported param descriptor type."));
    }
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorsCount),
                         writeDescriptorSets.data(), 0, nullptr);
}

ParamDescriptorUpdater::ParamDescriptorUpdater(ref_ptr<VulkanObjectManager> objectManager)
  : m_objectManager(std::move(objectManager))
{}

void ParamDescriptorUpdater::Reset()
{
  for (auto const & g : m_descriptorSetGroups)
    m_objectManager->DestroyDescriptorSetGroup(g);
  m_descriptorSetGroups.clear();
  m_descriptorSetIndex = 0;
  m_updateDescriptorFrame = 0;
}

void ParamDescriptorUpdater::Update(ref_ptr<dp::GraphicsContext> context)
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  if (m_program != vulkanContext->GetCurrentProgram())
  {
    Reset();
    m_program = vulkanContext->GetCurrentProgram();
  }

  // We can update descriptors only once per frame. So if we need to render
  // object several times per frame, we must allocate new descriptors.
  if (m_updateDescriptorFrame != vulkanContext->GetCurrentFrameIndex())
  {
    m_updateDescriptorFrame = vulkanContext->GetCurrentFrameIndex();
    m_descriptorSetIndex = 0;
  }
  else
  {
    m_descriptorSetIndex++;
  }

  CHECK_LESS_OR_EQUAL(m_descriptorSetIndex, m_descriptorSetGroups.size(), ());
  if (m_descriptorSetIndex == m_descriptorSetGroups.size())
    m_descriptorSetGroups.emplace_back(m_objectManager->CreateDescriptorSetGroup(m_program));

  m_descriptorSetGroups[m_descriptorSetIndex].Update(vulkanContext->GetDevice(),
                                                     vulkanContext->GetCurrentParamDescriptors());
}

VkDescriptorSet ParamDescriptorUpdater::GetDescriptorSet() const
{
  return m_descriptorSetGroups[m_descriptorSetIndex].m_descriptorSet;
}
}  // namespace vulkan
}  // namespace dp
