#include "drape/support_manager.hpp"
#include "drape/gl_functions.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include "std/target_os.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace dp
{
struct SupportManager::Configuration
{
  std::string m_deviceName;
  Version m_apiVersion;
  Version m_driverVersion;
};

char const * kSupportedAntialiasing = "Antialiasing";
char const * kVulkanForbidden = "VulkanForbidden";

void SupportManager::Init(ref_ptr<GraphicsContext> context)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_isInitialized)
    return;

  m_rendererName = context->GetRendererName();
  m_rendererVersion = context->GetRendererVersion();
  LOG(LINFO, ("Renderer =", m_rendererName, "| Api =", context->GetApiVersion(), "| Version =", m_rendererVersion));

  alohalytics::Stats::Instance().LogEvent("GPU", m_rendererName);

  m_isSamsungGoogleNexus = (m_rendererName == "PowerVR SGX 540" &&
                            m_rendererVersion.find("GOOGLENEXUS.ED945322") != std::string::npos);
  if (m_isSamsungGoogleNexus)
    LOG(LINFO, ("Samsung Google Nexus detected."));

  if (m_rendererName.find("Adreno") != std::string::npos)
  {
    std::vector<std::string> const models = { "200", "203", "205", "220", "225" };
    for (auto const & model : models)
    {
      if (m_rendererName.find(model) != std::string::npos)
      {
        LOG(LINFO, ("Adreno 200 device detected."));
        m_isAdreno200 = true;
        break;
      }
    }
  }

  m_isTegra = (m_rendererName.find("Tegra") != std::string::npos);
  if (m_isTegra)
    LOG(LINFO, ("NVidia Tegra device detected."));

  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES2 || apiVersion == dp::ApiVersion::OpenGLES3)
  {
    m_maxLineWidth = static_cast<float>(std::max(1, GLFunctions::glGetMaxLineWidth()));
    m_maxTextureSize = static_cast<uint32_t>(GLFunctions::glGetInteger(gl_const::GLMaxTextureSize));
  }
  else if (apiVersion == dp::ApiVersion::Metal)
  {
    // Metal does not support thick lines.
    m_maxLineWidth = 1.0f;
    m_maxTextureSize = 4096;
  }
  else if (apiVersion == dp::ApiVersion::Vulkan)
  {
    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    auto const & props = vulkanContext->GetGpuProperties();
    m_maxLineWidth = std::max(props.limits.lineWidthRange[0], props.limits.lineWidthRange[1]);
    m_maxTextureSize = vulkanContext->GetGpuProperties().limits.maxImageDimension2D;
  }
  LOG(LINFO, ("Max line width =", m_maxLineWidth,"| Max texture size =", m_maxTextureSize));

  // Set up default antialiasing value.
  // Turn off AA for a while by energy-saving issues.
//  bool val;
//  if (!settings::Get(kSupportedAntialiasing, val))
//  {
//#ifdef OMIM_OS_ANDROID
//    std::vector<std::string> const models = {"Mali-G71", "Mali-T880", "Adreno (TM) 540",
//                                             "Adreno (TM) 530", "Adreno (TM) 430"};
//    m_isAntialiasingEnabledByDefault =
//        (std::find(models.begin(), models.end(), m_rendererName) != models.end());
//#else
//    m_isAntialiasingEnabledByDefault = true;
//#endif
//    settings::Set(kSupportedAntialiasing, m_isAntialiasingEnabledByDefault);
//  }

  m_isInitialized = true;
}

void SupportManager::ForbidVulkan()
{
  alohalytics::Stats::Instance().LogEvent("VulkanForbidden", {{"GPU", m_rendererName},
                                                              {"Driver", m_rendererVersion}});
  settings::Set(kVulkanForbidden, true);
}

bool SupportManager::IsVulkanForbidden() const
{
  bool forbidden;
  if (!settings::Get(kVulkanForbidden, forbidden))
    forbidden = false;
  return forbidden;
}

bool SupportManager::IsVulkanForbidden(std::string const & deviceName,
                                       Version apiVersion, Version driverVersion) const
{
  static std::vector<std::string> const kBannedDevices = {"PowerVR Rogue GE8100",
                                                          "PowerVR Rogue GE8300"};

  // On these configurations we've detected fatal driver-specific Vulkan errors.
  static std::vector<Configuration> const kBannedConfigurations = {
    {"Adreno (TM) 506", {1, 0, 31}, {42, 264, 975}},
    {"Adreno (TM) 506", {1, 1, 66}, {512, 313, 0}},
    {"Adreno (TM) 530", {1, 1, 66}, {512, 313, 0}},
  };

  for (auto const & d : kBannedDevices)
  {
    if (d == deviceName)
      return true;
  }

  for (auto const & c : kBannedConfigurations)
  {
    if (c.m_deviceName == deviceName && c.m_apiVersion == apiVersion &&
        c.m_driverVersion == driverVersion)
    {
      return true;
    }
  }
  return false;
}

bool SupportManager::IsVulkanTexturePartialUpdateBuggy(int sdkVersion,
                                                       std::string const & deviceName,
                                                       Version apiVersion,
                                                       Version driverVersion) const
{
  int constexpr kMinSdkVersionForVulkan10 = 29;
  if (sdkVersion >= kMinSdkVersionForVulkan10)
    return true;

  // For these configurations partial updates of texture clears whole texture except part updated
  static std::vector<Configuration> const kBadConfigurations = {
      {"Mali-G76", {1, 1, 97}, {18, 0, 0}},
  };

  for (auto const & c : kBadConfigurations)
  {
    if (c.m_deviceName == deviceName && c.m_apiVersion == apiVersion &&
        c.m_driverVersion == driverVersion)
    {
      return true;
    }
  }
  return false;
}

SupportManager & SupportManager::Instance()
{
  static SupportManager manager;
  return manager;
}
}  // namespace dp
