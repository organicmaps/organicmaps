#include "shaders/vulkan_program_pool.hpp"
#include "shaders/program_params.hpp"

#include "drape/vulkan/vulkan_utils.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"

#include "base/logging.hpp"

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
std::string const kShadersDir = "vulkan_shaders";
std::string const kShadersReflecton = "reflection.json";

std::vector<uint32_t> ReadShaderFile(std::string const & filename)
{
  std::vector<uint32_t> result;
  try
  {
    ReaderPtr<Reader> reader(GetPlatform().GetReader(filename));
    auto const size = static_cast<size_t>(reader.Size());
    CHECK(size % sizeof(uint32_t) == 0,
          ("Incorrect SPIR_V file alignment. Name =", filename, "Size =", size));
    result.resize(size / sizeof(uint32_t));
    reader.Read(0, result.data(), size);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error reading shader file ", filename, " : ", e.what()));
    return {};
  }

  return result;
}

struct ReflectionData
{
  int32_t m_programIndex = -1;
  dp::vulkan::VulkanGpuProgram::ReflectionInfo m_info;
  DECLARE_VISITOR(visitor(m_programIndex, "prg"),
                  visitor(m_info, "info"))
};

struct ReflectionFile
{
  std::vector<ReflectionData> m_reflectionData;
  DECLARE_VISITOR(visitor(m_reflectionData))
};

std::map<uint8_t, dp::vulkan::VulkanGpuProgram::ReflectionInfo> ReadReflectionFile(
  std::string const & filename)
{
  std::map<uint8_t, dp::vulkan::VulkanGpuProgram::ReflectionInfo> result;
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
    result.insert(std::make_pair(d.m_programIndex, std::move(d.m_info)));

  return result;
}

VkShaderModule LoadShaderModule(VkDevice device, std::string const & filename)
{
  VkShaderModule shaderModule;

  auto src = ReadShaderFile(filename);
  if (src.empty())
    return {};

  auto const sizeInBytes = src.size() * sizeof(uint32_t);

  VkShaderModuleCreateInfo moduleCreateInfo = {};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.pNext = nullptr;
  moduleCreateInfo.codeSize = sizeInBytes;
  moduleCreateInfo.pCode = src.data();
  moduleCreateInfo.flags = 0;

  CHECK_VK_CALL(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));
  return shaderModule;
}
}  // namespace

VulkanProgramPool::VulkanProgramPool(VkDevice device)
  : m_device(device)
{
  auto reflection = ReadReflectionFile(base::JoinPath(kShadersDir, kShadersReflecton));
  CHECK_EQUAL(reflection.size(), static_cast<size_t>(Program::ProgramsCount), ());
  for (size_t i = 0; i < static_cast<size_t>(Program::ProgramsCount); ++i)
  {
    auto const programName = DebugPrint(static_cast<Program>(i));
    m_programs[i] = make_unique_dp<dp::vulkan::VulkanGpuProgram>(
      programName,
      std::move(reflection[i]),
      LoadShaderModule(device, base::JoinPath(kShadersDir, programName + ".vert.spv")),
      LoadShaderModule(device, base::JoinPath(kShadersDir, programName + ".frag.spv")));
  }

  ProgramParams::Init();
}

VulkanProgramPool::~VulkanProgramPool()
{
  ProgramParams::Destroy();

  for (auto & p : m_programs)
  {
    if (p != nullptr)
    {
      vkDestroyShaderModule(m_device, p->GetVertexShader(), nullptr);
      vkDestroyShaderModule(m_device, p->GetFragmentShader(), nullptr);
    }
  }
}

drape_ptr<dp::GpuProgram> VulkanProgramPool::Get(Program program)
{
  auto & p = m_programs[static_cast<size_t>(program)];
  CHECK(p != nullptr, ());
  return make_unique_dp<dp::vulkan::VulkanGpuProgram>(p->GetName(),
                                                      p->GetReflectionInfo(),
                                                      p->GetVertexShader(),
                                                      p->GetFragmentShader());
}
}  // namespace vulkan
}  // namespace gpu
