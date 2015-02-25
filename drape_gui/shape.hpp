#pragma once

#include "skin.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_manager.hpp"
#include "../drape/overlay_handle.hpp"
#include "../drape/glstate.hpp"
#include "../drape/vertex_array_buffer.hpp"
#include "../drape/gpu_program_manager.hpp"

namespace gui
{

class Shape
{
public:
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> tex) const = 0;
  virtual uint16_t GetVertexCount() const = 0;
  virtual uint16_t GetIndexCount() const = 0;

  void SetPosition(gui::Position const & position);

protected:
  gui::Position m_position;
};

class Handle : public dp::OverlayHandle
{
public:
  Handle(FeatureID const & id, dp::Anchor anchor, double p) : dp::OverlayHandle(id, anchor, p) {}
  dp::UniformValuesStorage const & GetUniforms() const { return m_uniforms; }

  void SetProjection(int w, int h);

protected:
  dp::UniformValuesStorage m_uniforms;
};

class ShapeRenderer
{
public:
  ShapeRenderer(dp::GLState const & state, dp::TransferPointer<dp::VertexArrayBuffer> buffer,
                dp::TransferPointer<dp::OverlayHandle> implHandle);
  ~ShapeRenderer();

  void Build(dp::RefPointer<dp::GpuProgramManager> mng);
  void Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng);

private:
  dp::GLState m_state;
  dp::MasterPointer<dp::VertexArrayBuffer> m_buffer;
  dp::MasterPointer<dp::OverlayHandle> m_implHandle;
};

}
