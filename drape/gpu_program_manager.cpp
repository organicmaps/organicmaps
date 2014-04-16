#include "gpu_program_manager.hpp"
#include "shader_def.hpp"

#include "../base/stl_add.hpp"
#include "../base/assert.hpp"

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
  (void)GetRangeDeletor(m_programs, MasterPointerDeleter())();
  (void)GetRangeDeletor(m_shaders, MasterPointerDeleter())();
}

RefPointer<GpuProgram> GpuProgramManager::GetProgram(int index)
{
  program_map_t::iterator it = m_programs.find(index);
  if (it != m_programs.end())
    return it->second.GetRefPointer();

  gpu::ProgramInfo const & programInfo = s_mapper.GetShaders(index);
  RefPointer<Shader> vertexShader = GetShader(programInfo.m_vertexIndex,
                                              programInfo.m_vertexSource,
                                              Shader::VertexShader);
  RefPointer<Shader> fragmentShader = GetShader(programInfo.m_fragmentIndex,
                                                programInfo.m_fragmentSource,
                                                Shader::FragmentShader);

  MasterPointer<GpuProgram> & result = m_programs[index];
  result.Reset(new GpuProgram(vertexShader, fragmentShader));
  return result.GetRefPointer();
}

RefPointer<Shader> GpuProgramManager::GetShader(int index, string const & source, Shader::Type t)
{
  shader_map_t::iterator it = m_shaders.find(index);
  if (it == m_shaders.end())
  {
    MasterPointer<Shader> & shader = m_shaders[index];
    shader.Reset(new Shader(source, t));
    return shader.GetRefPointer();
  }
  else
    return it->second.GetRefPointer();
}
