#include "drape/support_manager.hpp"
#include "drape/glfunctions.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <algorithm>
#include <string>
#include <vector>

namespace dp
{
char const * kSupportedAntialiasing = "Antialiasing";

void SupportManager::Init()
{
  std::string const renderer = GLFunctions::glGetString(gl_const::GLRenderer);
  std::string const version = GLFunctions::glGetString(gl_const::GLVersion);
  LOG(LINFO, ("Renderer =", renderer, "Api =", GLFunctions::CurrentApiVersion, "Driver version =", version));

  // On Android the engine may be recreated. Here we guarantee that GPU info is sent once per session.
  static bool gpuInfoSent = false;
  if (!gpuInfoSent)
  {
    alohalytics::Stats::Instance().LogEvent("GPU", renderer);
    gpuInfoSent = true;
  }

  m_isSamsungGoogleNexus = (renderer == "PowerVR SGX 540" && version.find("GOOGLENEXUS.ED945322") != string::npos);
  if (m_isSamsungGoogleNexus)
    LOG(LINFO, ("Samsung Google Nexus detected."));

  if (renderer.find("Adreno") != std::string::npos)
  {
    std::vector<std::string> const models = { "200", "203", "205", "220", "225" };
    for (auto const & model : models)
    {
      if (renderer.find(model) != std::string::npos)
      {
        LOG(LINFO, ("Adreno 200 device detected."));
        m_isAdreno200 = true;
        break;
      }
    }
  }

  m_isTegra = (renderer.find("Tegra") != std::string::npos);
  if (m_isTegra)
    LOG(LINFO, ("NVidia Tegra device detected."));

  m_maxLineWidth = std::max(1, GLFunctions::glGetMaxLineWidth());
  LOG(LINFO, ("Max line width =", m_maxLineWidth));

  // Set up default antialiasing value.
  // Turn off AA for a while by energy-saving issues.
//  bool val;
//  if (!settings::Get(kSupportedAntialiasing, val))
//  {
//#ifdef OMIM_OS_ANDROID
//    std::vector<std::string> const models = {"Mali-G71", "Mali-T880", "Adreno (TM) 540",
//                                             "Adreno (TM) 530", "Adreno (TM) 430"};
//    m_isAntialiasingEnabledByDefault =
//        (std::find(models.begin(), models.end(), renderer) != models.end());
//#else
//    m_isAntialiasingEnabledByDefault = true;
//#endif
//    settings::Set(kSupportedAntialiasing, m_isAntialiasingEnabledByDefault);
//  }
}

SupportManager & SupportManager::Instance()
{
  static SupportManager manager;
  return manager;
}
}  // namespace dp
