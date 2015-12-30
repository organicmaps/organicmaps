#include "drape/support_manager.hpp"
#include "drape/glfunctions.hpp"

#include "base/logging.hpp"

namespace dp
{

void SupportManager::Init()
{
  string const renderer = GLFunctions::glGetString(gl_const::GLRenderer);
  string const version = GLFunctions::glGetString(gl_const::GLVersion);
  LOG(LINFO, ("Renderer =", renderer, "Version =", version));

  m_isSamsungGoogleNexus = (renderer == "PowerVR SGX 540" && version.find("GOOGLENEXUS.ED945322") != string::npos);
  if (m_isSamsungGoogleNexus)
    LOG(LINFO, ("Samsung Google Nexus detected."));

  if (renderer.find("Adreno") != string::npos)
  {
    vector<string> const models = { "200", "203", "205", "220", "225" };
    for (auto const & model : models)
    {
      if (renderer.find(model) != string::npos)
      {
        LOG(LINFO, ("Adreno 200 device detected."));
        m_isAdreno200 = true;
        break;
      }
    }
  }

  m_isTegra = (renderer.find("Tegra") != string::npos);
  if (m_isTegra)
    LOG(LINFO, ("NVidia Tegra device detected."));
}

bool SupportManager::IsSamsungGoogleNexus() const
{
  return m_isSamsungGoogleNexus;
}

bool SupportManager::IsAdreno200Device() const
{
  return m_isAdreno200;
}

bool SupportManager::IsTegraDevice() const
{
  return m_isTegra;
}

SupportManager & SupportManager::Instance()
{
  static SupportManager manager;
  return manager;
}

} // namespace dp
