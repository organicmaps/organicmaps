#pragma once

#include "drape/pointers.hpp"

#include "std/vector.hpp"

namespace dp
{
class GpuProgram;
class GpuProgramManager;
}

class ScreenBase;

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

  uint32_t m_width = 0;
  uint32_t m_height = 0;

  uint32_t m_VAO = 0;
  uint32_t m_bufferId = 0;

  vector<float> m_vertices;
};

}  // namespace df
