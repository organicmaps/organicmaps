#pragma once

#include "skin.hpp"

#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glstate.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"
#include "drape/vertex_array_buffer.hpp"

namespace gui
{

class Handle : public dp::OverlayHandle
{
public:
  Handle(dp::Anchor anchor, m2::PointF const & pivot, m2::PointF const & size);

  dp::UniformValuesStorage const & GetUniforms() const { return m_uniforms; }

  void Update(ScreenBase const & screen) override;

  virtual bool IndexesRequired() const override;
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const override;
  virtual void GetPixelShape(ScreenBase const & screen, Rects & rects) const override;

  m2::PointF GetSize() const { return m_size; }
  void SetPivot(glsl::vec2 const & pivot) { m_pivot = pivot; }

protected:
  dp::UniformValuesStorage m_uniforms;
  glsl::vec2 m_pivot;
  mutable m2::PointF m_size;
};

struct ShapeControl
{
  ShapeControl() = default;
  ShapeControl(ShapeControl const & other) = delete;
  ShapeControl(ShapeControl && other) = default;

  ShapeControl & operator=(ShapeControl const & other) = delete;
  ShapeControl & operator=(ShapeControl && other) = default;

  struct ShapeInfo
  {
    ShapeInfo() : m_state(0, dp::GLState::Gui) {}
    ShapeInfo(dp::GLState const & state, dp::TransferPointer<dp::VertexArrayBuffer> buffer,
              dp::TransferPointer<Handle> handle);

    void Destroy();

    dp::GLState m_state;
    dp::MasterPointer<dp::VertexArrayBuffer> m_buffer;
    dp::MasterPointer<Handle> m_handle;
  };

  void AddShape(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket);

  vector<ShapeInfo> m_shapesInfo;
};

class ShapeRenderer final
{
public:
  using TShapeControlEditFn = function<void(ShapeControl &)>;

  ~ShapeRenderer();

  void Build(dp::RefPointer<dp::GpuProgramManager> mng);
  void Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng);
  void AddShape(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket);
  void AddShapeControl(ShapeControl && control);

private:
  friend void ArrangeShapes(dp::RefPointer<ShapeRenderer>,
                            ShapeRenderer::TShapeControlEditFn const &);
  void ForEachShapeControl(TShapeControlEditFn const & fn);

  using TShapeInfoEditFn = function<void(ShapeControl::ShapeInfo &)>;
  void ForEachShapeInfo(TShapeInfoEditFn const & fn);

private:
  vector<ShapeControl> m_shapes;
};

void ArrangeShapes(dp::RefPointer<ShapeRenderer> renderer,
                   ShapeRenderer::TShapeControlEditFn const & fn);

class Shape
{
public:
  Shape(gui::Position const & position) : m_position(position) {}
  virtual ~Shape() {}

  virtual dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const = 0;

protected:
  gui::Position m_position;
};

}
