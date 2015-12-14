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

class TransparentLayer
{
public:
  TransparentLayer();
  ~TransparentLayer();

  void Render(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  uint32_t m_bufferId = 0;
  int8_t m_attributePosition;
  int8_t m_attributeTexCoord;

  vector<float> m_vertices;
};

}  // namespace df
