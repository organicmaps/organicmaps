#pragma once

#include "drape/pointers.hpp"
#include "drape/gpu_program.hpp"
#include "drape/shader.hpp"

#include "std/map.hpp"
#include "std/noncopyable.hpp"

namespace dp
{

class GpuProgramManager : public noncopyable
{
public:
  ~GpuProgramManager();

  ref_ptr<GpuProgram> GetProgram(int index);

private:
  ref_ptr<Shader> GetShader(int index, string const & source, Shader::Type t);

private:
  typedef map<int, drape_ptr<GpuProgram> > program_map_t;
  typedef map<int, drape_ptr<Shader> > shader_map_t;
  program_map_t m_programs;
  shader_map_t m_shaders;
};

} // namespace dp
