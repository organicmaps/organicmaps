#include "drape/gpu_program_manager.hpp"
#include "drape/glfunctions.hpp"
#include "drape/support_manager.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"

namespace dp
{
GpuProgramManager::~GpuProgramManager()
{
  m_programs.clear();
  m_shaders.clear();
}

void GpuProgramManager::Init(drape_ptr<gpu::GpuProgramGetter> && programGetter)
{
  m_programGetter = std::move(programGetter);
  ASSERT(m_programGetter != nullptr, ());

  // This feature is not supported on some Android devices (especially on Android 4.x version).
  // Since we can't predict on which devices it'll work fine, we have to turn off for all devices.
#if !defined(OMIM_OS_ANDROID)
  if (GLFunctions::glGetInteger(gl_const::GLMaxVertexTextures) > 0)
  {
    LOG(LINFO, ("VTF enabled"));
    m_globalDefines.append("#define ENABLE_VTF\n");  // VTF == Vetrex Texture Fetch
  }
#endif

  if (SupportManager::Instance().IsSamsungGoogleNexus())
    m_globalDefines.append("#define SAMSUNG_GOOGLE_NEXUS\n");

  if (GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES3)
    m_globalDefines.append("#define GLES3\n");
}

ref_ptr<GpuProgram> GpuProgramManager::GetProgram(int index)
{
  auto it = m_programs.find(index);
  if (it != m_programs.end())
    return make_ref(it->second);

  auto const & programInfo = m_programGetter->GetProgramInfo(index);
  auto vertexShader = GetShader(programInfo.m_vertexIndex, programInfo.m_vertexSource,
                                Shader::Type::VertexShader);
  auto fragmentShader = GetShader(programInfo.m_fragmentIndex, programInfo.m_fragmentSource,
                                  Shader::Type::FragmentShader);

  drape_ptr<GpuProgram> program = make_unique_dp<GpuProgram>(index, vertexShader, fragmentShader,
                                                             programInfo.m_textureSlotsCount);
  ref_ptr<GpuProgram> result = make_ref(program);
  m_programs.emplace(index, move(program));

  return result;
}

ref_ptr<Shader> GpuProgramManager::GetShader(int index, string const & source, Shader::Type t)
{
  auto it = m_shaders.find(index);
  if (it != m_shaders.end())
    return make_ref(it->second);

  drape_ptr<Shader> shader = make_unique_dp<Shader>(source, m_globalDefines, t);
  ref_ptr<Shader> result = make_ref(shader);
  m_shaders.emplace(index, move(shader));
  return result;
}
}  // namespace dp
