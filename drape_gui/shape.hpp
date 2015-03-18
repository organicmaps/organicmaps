#pragma once

#include "skin.hpp"

#include "../drape/batcher.hpp"
#include "../drape/glsl_types.hpp"
#include "../drape/glstate.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/overlay_handle.hpp"
#include "../drape/texture_manager.hpp"
#include "../drape/vertex_array_buffer.hpp"

namespace gui
{

class Handle : public dp::OverlayHandle
{
public:
  Handle(dp::Anchor anchor, m2::PointF const & pivot)
    : dp::OverlayHandle(FeatureID(), anchor, 0.0)
    , m_pivot(glsl::ToVec2(pivot))
  {
  }

  dp::UniformValuesStorage const & GetUniforms() const { return m_uniforms; }

  virtual bool IndexesRequired() const override;
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const override;
  virtual void GetPixelShape(ScreenBase const & screen, Rects & rects) const override;

protected:
  dp::UniformValuesStorage m_uniforms;
  glsl::vec2 const m_pivot;
};

class ShapeRenderer final
{
public:
  ~ShapeRenderer();

  void Build(dp::RefPointer<dp::GpuProgramManager> mng);
  void Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng);
  void AddShape(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket);

private:
  struct ShapeInfo
  {
    ShapeInfo(dp::GLState const & state, dp::TransferPointer<dp::VertexArrayBuffer> buffer,
              dp::TransferPointer<dp::OverlayHandle> handle);

    void Destroy();

    dp::GLState m_state;
    dp::MasterPointer<dp::VertexArrayBuffer> m_buffer;
    dp::MasterPointer<dp::OverlayHandle> m_handle;
  };

  vector<ShapeInfo> m_shapes;
};

class Shape
{
public:
  virtual ~Shape() {}

  virtual dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const = 0;
  void SetPosition(gui::Position const & position);

protected:
  gui::Position m_position;
};

}
