#pragma once

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

namespace dp
{
class GpuProgram;
class GpuProgramManager;
}  // namespace dp

namespace df
{
class ScreenQuadRenderer
{
public:
  ScreenQuadRenderer() = default;
  ~ScreenQuadRenderer();

  void SetTextureRect(m2::RectF const & rect, ref_ptr<dp::GpuProgramManager> mng);
  bool IsInitialized() const { return m_bufferId != 0; }
  m2::RectF const & GetTextureRect() const { return m_textureRect; }
  void RenderTexture(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng, float opacity);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  uint32_t m_bufferId = 0;
  uint32_t m_VAO = 0;
  int8_t m_attributePosition = -1;
  int8_t m_attributeTexCoord = -1;
  int8_t m_textureLocation = -1;
  m2::RectF m_textureRect = m2::RectF(0.0f, 0.0f, 1.0f, 1.0f);
};
}  // namespace df
