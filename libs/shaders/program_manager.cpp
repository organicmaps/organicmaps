#include "shaders/program_manager.hpp"
#include "shaders/gl_program_pool.hpp"
#include "shaders/vulkan_program_params.hpp"
#include "shaders/vulkan_program_pool.hpp"

#include "drape/gl_functions.hpp"
#include "drape/support_manager.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

#include "base/logging.hpp"

#include "gl_program_params.hpp"

#include <algorithm>

namespace gpu
{
void ProgramManager::Init(ref_ptr<dp::GraphicsContext> context)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    InitForOpenGL(context);
  }
  else if (apiVersion == dp::ApiVersion::Metal)
  {
#if defined(OMIM_METAL_AVAILABLE)
    InitForMetal(context);
#endif
  }
  else if (apiVersion == dp::ApiVersion::Vulkan)
  {
    InitForVulkan(context);
  }
  else
  {
    CHECK(false, ("Unsupported API version."));
  }
}

void ProgramManager::Destroy(ref_ptr<dp::GraphicsContext> context)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::Metal)
  {
#if defined(OMIM_METAL_AVAILABLE)
    DestroyForMetal(context);
#endif
  }
  else if (apiVersion == dp::ApiVersion::Vulkan)
  {
    DestroyForVulkan(context);
  }
}

void ProgramManager::InitForOpenGL(ref_ptr<dp::GraphicsContext> context)
{
  std::string globalDefines;

  // This feature is not supported on some Android devices (especially on Android 4.x version).
  // Since we can't predict on which devices it'll work fine, we have to turn off for all devices.
#if !defined(OMIM_OS_ANDROID)
  if (GLFunctions::glGetInteger(gl_const::GLMaxVertexTextures) > 0)
  {
    LOG(LINFO, ("VTF enabled"));
    globalDefines.append("#define ENABLE_VTF\n");  // VTF == Vertex Texture Fetch
  }
#endif

  auto const apiVersion = context->GetApiVersion();
  m_pool = make_unique_dp<GLProgramPool>(apiVersion);
  ref_ptr<GLProgramPool> pool = make_ref(m_pool);
  pool->SetDefines(globalDefines);

  m_paramsSetter = make_unique_dp<GLProgramParamsSetter>();
}

void ProgramManager::InitForVulkan(ref_ptr<dp::GraphicsContext> context)
{
  m_pool = make_unique_dp<vulkan::VulkanProgramPool>(context);
  m_paramsSetter = make_unique_dp<vulkan::VulkanProgramParamsSetter>(
      context, make_ref(static_cast<vulkan::VulkanProgramPool *>(m_pool.get())));
}

void ProgramManager::DestroyForVulkan(ref_ptr<dp::GraphicsContext> context)
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  vulkanContext->ResetPipelineCache();

  ASSERT(dynamic_cast<vulkan::VulkanProgramParamsSetter *>(m_paramsSetter.get()) != nullptr, ());
  static_cast<vulkan::VulkanProgramParamsSetter *>(m_paramsSetter.get())->Destroy(vulkanContext);

  ASSERT(dynamic_cast<vulkan::VulkanProgramPool *>(m_pool.get()) != nullptr, ());
  static_cast<vulkan::VulkanProgramPool *>(m_pool.get())->Destroy(vulkanContext);
}

ref_ptr<dp::GpuProgram> ProgramManager::GetProgram(Program program)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto & programPtr = m_programs[static_cast<size_t>(program)];
  if (programPtr)
    return make_ref(programPtr);

  programPtr = m_pool->Get(program);
  return make_ref(programPtr);
}

ref_ptr<ProgramParamsSetter> ProgramManager::GetParamsSetter() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return make_ref(m_paramsSetter);
}
}  // namespace gpu
