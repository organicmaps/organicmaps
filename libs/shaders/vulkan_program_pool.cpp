#include "shaders/vulkan_program_pool.hpp"
#include "shaders/program_params.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace gpu
{
namespace vulkan
{
namespace
{
int8_t constexpr kInvalidBindingIndex = -1;
uint32_t constexpr kMaxUniformBuffers = 1;

std::string const kShadersDir = "vulkan_shaders";
std::string const kShadersReflection = "reflection.json";
std::string const kShadersPackFile = "shaders_pack.spv";

std::vector<uint8_t> ReadShadersPackFile(std::string const & filename)
{
  std::vector<uint8_t> result;
  try
  {
    ReaderPtr<Reader> reader(GetPlatform().GetReader(filename));
    auto const size = static_cast<size_t>(reader.Size());
    CHECK(size % sizeof(uint32_t) == 0, ("Incorrect SPIR-V file alignment."));
    result.resize(size);
    reader.Read(0, result.data(), size);
  }
  catch (RootException const & e)
  {
    CHECK(false, ("Error reading shader file ", filename, " : ", e.what()));
    return {};
  }
  return result;
}

struct TextureBindingReflectionInfo
{
  std::string m_name;
  int8_t m_index = kInvalidBindingIndex;
  int8_t m_isInFragmentShader = 1;
  DECLARE_VISITOR(visitor(m_name, "name"), visitor(m_index, "idx"), visitor(m_isInFragmentShader, "frag"))
};

struct ReflectionInfo
{
  int8_t m_vsUniformsIndex = kInvalidBindingIndex;
  int8_t m_fsUniformsIndex = kInvalidBindingIndex;
  std::vector<TextureBindingReflectionInfo> m_textures;

  DECLARE_VISITOR(visitor(m_vsUniformsIndex, "vs_uni"), visitor(m_fsUniformsIndex, "fs_uni"),
                  visitor(m_textures, "tex"))
};

struct ReflectionData
{
  int32_t m_programIndex = -1;
  uint32_t m_vsOffset = 0;
  uint32_t m_fsOffset = 0;
  uint32_t m_vsSize = 0;
  uint32_t m_fsSize = 0;
  ReflectionInfo m_info;
  DECLARE_VISITOR(visitor(m_programIndex, "prg"), visitor(m_vsOffset, "vs_off"), visitor(m_fsOffset, "fs_off"),
                  visitor(m_vsSize, "vs_size"), visitor(m_fsSize, "fs_size"), visitor(m_info, "info"))
};

struct ReflectionFile
{
  std::vector<ReflectionData> m_reflectionData;
  DECLARE_VISITOR(visitor(m_reflectionData))
};

std::map<uint8_t, ReflectionData> ReadReflectionFile(std::string const & filename)
{
  std::map<uint8_t, ReflectionData> result;
  std::string jsonStr;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(filename)).ReadAsString(jsonStr);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error reading file ", filename, ":", e.what()));
    return {};
  }

  ReflectionFile reflectionFile;
  try
  {
    coding::DeserializerJson des(jsonStr);
    des(reflectionFile);
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, ("Error deserialization reflection file :", e.what()));
    return {};
  }

  for (auto & d : reflectionFile.m_reflectionData)
  {
    auto const index = d.m_programIndex;
    std::ranges::sort(d.m_info.m_textures, [](auto const & a, auto const & b) { return a.m_index < b.m_index; });
    result.insert(std::make_pair(index, std::move(d)));
  }

  return result;
}

std::vector<VkDescriptorSetLayoutBinding> GetLayoutBindings(ReflectionInfo const & reflectionInfo,
                                                            uint32_t maxTextureBindings)
{
  std::vector<VkDescriptorSetLayoutBinding> result;

  VkDescriptorSetLayoutBinding uniformsBinding = {};
  uniformsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uniformsBinding.descriptorCount = 1;

  if (reflectionInfo.m_vsUniformsIndex != kInvalidBindingIndex &&
      reflectionInfo.m_fsUniformsIndex != kInvalidBindingIndex)
  {
    CHECK_EQUAL(reflectionInfo.m_vsUniformsIndex, reflectionInfo.m_fsUniformsIndex, ());
    uniformsBinding.binding = static_cast<uint32_t>(reflectionInfo.m_vsUniformsIndex);
    uniformsBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    result.push_back(std::move(uniformsBinding));
  }
  else if (reflectionInfo.m_vsUniformsIndex != kInvalidBindingIndex)
  {
    uniformsBinding.binding = static_cast<uint32_t>(reflectionInfo.m_vsUniformsIndex);
    uniformsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    result.push_back(std::move(uniformsBinding));
  }
  else if (reflectionInfo.m_fsUniformsIndex != kInvalidBindingIndex)
  {
    uniformsBinding.binding = static_cast<uint32_t>(reflectionInfo.m_fsUniformsIndex);
    uniformsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    result.push_back(std::move(uniformsBinding));
  }
  else
  {
    CHECK(false, ("Uniforms must be at least in one shader."));
  }

  for (auto const & t : reflectionInfo.m_textures)
  {
    CHECK_GREATER_OR_EQUAL(t.m_index, 0, ());
    VkDescriptorSetLayoutBinding textureBinding = {};
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.binding = static_cast<uint32_t>(t.m_index);
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags =
        (t.m_isInFragmentShader == 1) ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
    result.push_back(std::move(textureBinding));
  }

  // Add empty bindings for unused texture slots.
  uint32_t bindingIndex = 1;
  if (!reflectionInfo.m_textures.empty())
    bindingIndex = static_cast<uint32_t>(reflectionInfo.m_textures.back().m_index) + 1;

  for (uint32_t i = static_cast<uint32_t>(reflectionInfo.m_textures.size()); i < maxTextureBindings; ++i)
    result.push_back({
        .binding = bindingIndex++,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });

  return result;
}

VkShaderModule LoadShaderModule(VkDevice device, std::vector<uint8_t> const & packData, uint32_t offset, uint32_t size)
{
  VkShaderModule shaderModule;
  CHECK(offset % sizeof(uint32_t) == 0, ("Incorrect SPIR-V file alignment."));
  CHECK(size % sizeof(uint32_t) == 0, ("Incorrect SPIR-V file size."));

  VkShaderModuleCreateInfo moduleCreateInfo = {};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.pNext = nullptr;
  moduleCreateInfo.codeSize = size;
  moduleCreateInfo.pCode = reinterpret_cast<uint32_t const *>(packData.data() + offset);
  moduleCreateInfo.flags = 0;

  CHECK_VK_CALL(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));
  return shaderModule;
}
}  // namespace

VulkanProgramPool::VulkanProgramPool(ref_ptr<dp::GraphicsContext> context)
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkDevice device = vulkanContext->GetDevice();

  auto reflection = ReadReflectionFile(base::JoinPath(kShadersDir, kShadersReflection));
  CHECK_EQUAL(reflection.size(), static_cast<size_t>(Program::ProgramsCount), ());

  auto packFileData = ReadShadersPackFile(base::JoinPath(kShadersDir, kShadersPackFile));

  for (size_t i = 0; i < static_cast<size_t>(Program::ProgramsCount); ++i)
    m_maxImageSamplers = std::max(m_maxImageSamplers, static_cast<uint32_t>(reflection[i].m_info.m_textures.size()));

  for (size_t i = 0; i < static_cast<size_t>(Program::ProgramsCount); ++i)
  {
    auto const & refl = reflection[i];
    m_programData[i].m_vertexShader = LoadShaderModule(device, packFileData, refl.m_vsOffset, refl.m_vsSize);
    m_programData[i].m_fragmentShader = LoadShaderModule(device, packFileData, refl.m_fsOffset, refl.m_fsSize);
    auto const bindings = GetLayoutBindings(refl.m_info, m_maxImageSamplers);
    CHECK(bindings.size() == kMaxUniformBuffers + m_maxImageSamplers, ("Incorrect bindings count."));

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pBindings = bindings.data();
    descriptorLayout.bindingCount = static_cast<uint32_t>(bindings.size());

    CHECK_VK_CALL(
        vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_programData[i].m_descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_programData[i].m_descriptorSetLayout;

    CHECK_VK_CALL(
        vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_programData[i].m_pipelineLayout));

    for (auto const & t : refl.m_info.m_textures)
      m_programData[i].m_textureBindings[t.m_name] = t.m_index;
  }

  ProgramParams::Init();
}

VulkanProgramPool::~VulkanProgramPool()
{
  ProgramParams::Destroy();
}

void VulkanProgramPool::Destroy(ref_ptr<dp::GraphicsContext> context)
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkDevice device = vulkanContext->GetDevice();

  for (auto & d : m_programData)
  {
    vkDestroyPipelineLayout(device, d.m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, d.m_descriptorSetLayout, nullptr);
    vkDestroyShaderModule(device, d.m_vertexShader, nullptr);
    vkDestroyShaderModule(device, d.m_fragmentShader, nullptr);
  }
}

drape_ptr<dp::GpuProgram> VulkanProgramPool::Get(Program program)
{
  auto const & d = m_programData[static_cast<size_t>(program)];

  VkPipelineShaderStageCreateInfo vsStage = {};
  vsStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vsStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vsStage.module = d.m_vertexShader;
  vsStage.pName = "main";

  VkPipelineShaderStageCreateInfo fsStage = {};
  fsStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fsStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fsStage.module = d.m_fragmentShader;
  fsStage.pName = "main";

  return make_unique_dp<dp::vulkan::VulkanGpuProgram>(DebugPrint(program), vsStage, fsStage, d.m_descriptorSetLayout,
                                                      d.m_pipelineLayout, d.m_textureBindings);
}

uint32_t VulkanProgramPool::GetMaxUniformBuffers() const
{
  return kMaxUniformBuffers;
}

uint32_t VulkanProgramPool::GetMaxImageSamplers() const
{
  return m_maxImageSamplers;
}
}  // namespace vulkan
}  // namespace gpu
