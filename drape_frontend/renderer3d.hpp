#pragma once

#include "drape/gpu_program_manager.hpp"

#include "geometry/screenbase.hpp"

namespace df
{

class Renderer3d
{
public:
  Renderer3d();
  ~Renderer3d();

  void SetSize(uint32_t width, uint32_t height);
  void Render(ScreenBase const & screen, uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  uint32_t m_width;
  uint32_t m_height;

  uint32_t m_VAO;
  uint32_t m_bufferId;

  array<float, 16> m_vertices;
};

}
