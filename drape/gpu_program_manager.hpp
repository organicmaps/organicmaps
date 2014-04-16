#pragma once

#include "pointers.hpp"
#include "gpu_program.hpp"
#include "shader.hpp"

#include "../std/map.hpp"
#include "../std/noncopyable.hpp"

class GpuProgramManager : public noncopyable
{
public:
  ~GpuProgramManager();

  RefPointer<GpuProgram> GetProgram(int index);

private:
  RefPointer<Shader> GetShader(int index, string const & source, Shader::Type t);

private:
  typedef map<int, MasterPointer<GpuProgram> > program_map_t;
  typedef map<int, MasterPointer<Shader> > shader_map_t;
  program_map_t m_programs;
  shader_map_t m_shaders;
};
