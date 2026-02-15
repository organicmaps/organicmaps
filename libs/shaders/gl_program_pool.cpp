#include "base/assert.hpp"

#include "shaders/gl_program_pool.hpp"
#include "shaders/gl_shaders.hpp"
#include "shaders/program_params.hpp"

#include "drape/gl_functions.hpp"
#include "drape/gl_gpu_program.hpp"

#include "std/target_os.hpp"

namespace gpu
{
GLProgramPool::GLProgramPool(dp::ApiVersion apiVersion, std::string_view additionalDefines) : m_apiVersion(apiVersion)
{
  ProgramParams::Init();

  if (m_apiVersion == dp::ApiVersion::OpenGLES3)
  {
#if defined(OMIM_OS_DESKTOP)
    m_defines = std::string(GL3_SHADER_VERSION);
#else
    m_defines = std::string(GLES3_SHADER_VERSION);
#endif
    m_defines.append(additionalDefines);
  }
}

GLProgramPool::~GLProgramPool()
{
  GLFunctions::glUseProgram(0);
  ProgramParams::Destroy();
}

drape_ptr<dp::GpuProgram> GLProgramPool::Get(Program program)
{
  auto const programInfo = GetProgramInfo(m_apiVersion, program);
  auto vertexShader =
      GetShader(programInfo.m_vertexShaderName, programInfo.m_vertexShaderSource, dp::Shader::Type::VertexShader);
  auto fragmentShader =
      GetShader(programInfo.m_fragmentShaderName, programInfo.m_fragmentShaderSource, dp::Shader::Type::FragmentShader);

  auto const name = DebugPrint(program);
  return make_unique_dp<dp::GLGpuProgram>(name, vertexShader, fragmentShader);
}

ref_ptr<dp::Shader> GLProgramPool::GetShader(std::string const & name, std::string const & source,
                                             dp::Shader::Type type)
{
  if (auto const it = m_shaders.find(name); it != m_shaders.end())
    return make_ref(it->second);

  auto [it, inserted] = m_shaders.emplace(name, make_unique_dp<dp::Shader>(name, source, m_defines, type));
  ASSERT(inserted, ("Shader with name", name, "already exists in the pool"));
  return make_ref(it->second);
}
}  // namespace gpu
