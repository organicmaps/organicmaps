#pragma once

#include "pointers.hpp"
#include "gpu_program.hpp"
#include "shader_reference.hpp"

#include "../std/map.hpp"
#include "../std/noncopyable.hpp"

class GpuProgramManager : public noncopyable
{
public:
  GpuProgramManager();
  ~GpuProgramManager();

  ReferencePoiner<GpuProgram> GetProgram(int index);

private:
  ReferencePoiner<ShaderReference> GetShader(int index, const string & source, ShaderReference::Type t);

private:
  typedef map<int, OwnedPointer<GpuProgram> > program_map_t;
  typedef map<int, OwnedPointer<ShaderReference> > shader_map_t;
  program_map_t m_programs;
  shader_map_t m_shaders;
};
