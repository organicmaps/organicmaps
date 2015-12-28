#include "drape/gpu_program_manager.hpp"
#include "drape/shader_def.hpp"
#include "drape/support_manager.hpp"

#include "base/stl_add.hpp"
#include "base/assert.hpp"
#include "base/logging.hpp"

namespace dp
{

namespace
{

class ShaderMapper
{
public:
  ShaderMapper()
  {
    gpu::InitGpuProgramsLib(m_mapping);
  }

  gpu::ProgramInfo const & GetShaders(int program) const
  {
    map<int, gpu::ProgramInfo>::const_iterator it = m_mapping.find(program);
    ASSERT(it != m_mapping.end(), ());
    return it->second;
  }

private:
  map<int, gpu::ProgramInfo> m_mapping;
};

static ShaderMapper s_mapper;

} // namespace

GpuProgramManager::~GpuProgramManager()
{
  m_programs.clear();
  m_shaders.clear();
}

void GpuProgramManager::Init()
{
  // This feature is not supported on some Android devices (especially on Android 4.x version).
  // Since we can't predict on which devices it'll work fine, we have to turn off for all devices.
#if !defined(OMIM_OS_ANDROID)
  if (GLFunctions::glGetInteger(gl_const::GLMaxVertexTextures) > 0)
  {
    LOG(LINFO, ("VTF enabled"));
    m_globalDefines.append("#define ENABLE_VTF\n"); // VTF == Vetrex Texture Fetch
  }
#endif

  if (SupportManager::Instance().IsSamsungGoogleNexus())
    m_globalDefines.append("#define SAMSUNG_GOOGLE_NEXUS\n");
}

ref_ptr<GpuProgram> GpuProgramManager::GetProgram(int index)
{
  program_map_t::iterator it = m_programs.find(index);
  if (it != m_programs.end())
    return make_ref(it->second);

  gpu::ProgramInfo const & programInfo = s_mapper.GetShaders(index);
  ref_ptr<Shader> vertexShader = GetShader(programInfo.m_vertexIndex,
                                           programInfo.m_vertexSource,
                                           Shader::VertexShader);
  ref_ptr<Shader> fragmentShader = GetShader(programInfo.m_fragmentIndex,
                                             programInfo.m_fragmentSource,
                                             Shader::FragmentShader);

  drape_ptr<GpuProgram> program = make_unique_dp<GpuProgram>(vertexShader, fragmentShader);
  ref_ptr<GpuProgram> result = make_ref(program);
  m_programs.emplace(index, move(program));

  return result;
}

ref_ptr<Shader> GpuProgramManager::GetShader(int index, string const & source, Shader::Type t)
{
  shader_map_t::iterator it = m_shaders.find(index);
  if (it == m_shaders.end())
  {
    drape_ptr<Shader> shader = make_unique_dp<Shader>(source, m_globalDefines, t);
    ref_ptr<Shader> result = make_ref(shader);
    m_shaders.emplace(index, move(shader));
    return result;
  }
  return make_ref(it->second);
}

} // namespace dp
