#include "gpu_program_manager.hpp"
#include "../base/assert.hpp"

#include "shader_def.hpp"

namespace
{
  class ShaderMapper
  {
  public:
    ShaderMapper()
    {
      gpu::InitGpuProgramsLib(m_mapping);
    }

    const gpu::ProgramInfo & GetShaders(int program) const
    {
      map<int, gpu::ProgramInfo>::const_iterator it = m_mapping.find(program);
      ASSERT(it != m_mapping.end(), ());
      return it->second;
    }

  private:
    map<int, gpu::ProgramInfo> m_mapping;
  };

  static ShaderMapper s_mapper;
}

GpuProgramManager::GpuProgramManager()
{
}

GpuProgramManager::~GpuProgramManager()
{
  shader_map_t::iterator sit = m_shaders.begin();
  for (; sit != m_shaders.end(); ++sit)
  {
    sit->second->Deref();
    sit->second.Destroy();
  }

  program_map_t::iterator pit = m_programs.begin();
  for (; pit != m_programs.end(); ++pit)
    pit->second.Destroy();
}

ReferencePoiner<GpuProgram> GpuProgramManager::GetProgram(int index)
{
  program_map_t::iterator it = m_programs.find(index);
  if (it != m_programs.end())
    return it->second.GetWeakPointer();

  gpu::ProgramInfo const & programInfo = s_mapper.GetShaders(index);
  ReferencePoiner<ShaderReference> vertexShader = GetShader(programInfo.m_vertexIndex,
                                                            programInfo.m_vertexSource,
                                                            ShaderReference::VertexShader);
  ReferencePoiner<ShaderReference> fragmentShader = GetShader(programInfo.m_fragmentIndex,
                                                              programInfo.m_fragmentSource,
                                                              ShaderReference::FragmentShader);

  OwnedPointer<GpuProgram> p(new GpuProgram(vertexShader, fragmentShader));
  m_programs.insert(std::make_pair(index, p));
  return p.GetWeakPointer();
}

ReferencePoiner<ShaderReference> GpuProgramManager::GetShader(int index, const string & source, ShaderReference::Type t)
{
  shader_map_t::iterator it = m_shaders.find(index);
  if (it == m_shaders.end())
  {
    OwnedPointer<ShaderReference> r(new ShaderReference(source, t));
    r->Ref();
    m_shaders.insert(std::make_pair(index, r));
  }

  return m_shaders[index].GetWeakPointer();
}
