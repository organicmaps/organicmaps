#include "gpu_program_manager.hpp"
#include "../base/assert.hpp"

namespace
{
  const string SimpleVertexShader =
      "attribute vec2 position;\
       attribute float depth;\
       attribute vec4 color; \
       uniform mat4 modelViewMatrix;\
       uniform mat4 projectionMatrix;\
       varying vec4 vColor; \
       void main()\
       {\
         gl_Position = vec4(position.xy, depth, 1.0) * modelViewMatrix * projectionMatrix;\
         vColor = color;\
       }";

  const string SimpleFragmentShader =
       "varying vec4 vColor;\
        void main()\
        {\
          gl_FragColor = vColor;\
        }";

  struct ShadersInfo
  {
    int32_t VertexShaderIndex;
    int32_t FragmentShaderIndex;
    string VertexShaderSource;
    string FragmentShaderSource;
  };

  class ShaderMapper
  {
  public:
    ShaderMapper()
    {
      ShadersInfo info;
      info.VertexShaderIndex = 1;
      info.FragmentShaderIndex = 2;
      info.FragmentShaderSource = SimpleFragmentShader;
      info.VertexShaderSource = SimpleVertexShader;
      m_mapping[1] = info;
    }

    const ShadersInfo & GetShaders(int program)
    {
      ASSERT(m_mapping.find(program) != m_mapping.end(), ());
      return m_mapping[program];
    }

  private:
    map<int, ShadersInfo> m_mapping;
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

  ShadersInfo const & shaders = s_mapper.GetShaders(index);
  ReferencePoiner<ShaderReference> vertexShader = GetShader(shaders.VertexShaderIndex,
                                                        shaders.VertexShaderSource,
                                                        ShaderReference::VertexShader);
  ReferencePoiner<ShaderReference> fragmentShader = GetShader(shaders.FragmentShaderIndex,
                                                          shaders.FragmentShaderSource,
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
