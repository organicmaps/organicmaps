#pragma once

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include <string>

namespace dp
{
class GpuProgram;
class GpuProgramManager;
}  // namespace dp

namespace df
{
class RendererContext
{
public:
  virtual ~RendererContext() {}
  virtual int GetGpuProgram() const = 0;
  virtual void PreRender(ref_ptr<dp::GpuProgram> prg) {}
  virtual void PostRender() {}
protected:
  void BindTexture(uint32_t textureId, ref_ptr<dp::GpuProgram> prg,
                   std::string const & uniformName, uint8_t slotIndex,
                   uint32_t filteringMode, uint32_t wrappingMode);
};

class ScreenQuadRenderer
{
public:
  ScreenQuadRenderer();
  ~ScreenQuadRenderer();

  void SetTextureRect(m2::RectF const & rect, ref_ptr<dp::GpuProgram> prg);
  void Rebuild(ref_ptr<dp::GpuProgram> prg);

  bool IsInitialized() const { return m_bufferId != 0; }
  m2::RectF const & GetTextureRect() const { return m_textureRect; }

  void Render(ref_ptr<dp::GpuProgramManager> mng, ref_ptr<RendererContext> context);
  void RenderTexture(ref_ptr<dp::GpuProgramManager> mng, uint32_t textureId, float opacity);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  uint32_t m_bufferId = 0;
  uint32_t m_VAO = 0;
  int8_t m_attributePosition = -1;
  int8_t m_attributeTexCoord = -1;
  m2::RectF m_textureRect = m2::RectF(0.0f, 0.0f, 1.0f, 1.0f);

  drape_ptr<RendererContext> m_textureRendererContext;
};
}  // namespace df
