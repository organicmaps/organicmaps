#pragma once

#include "drape/debug_renderer.hpp"
#include "drape/gpu_program.hpp"
#include "drape/mesh_object.hpp"
#include "drape/render_state.hpp"

#include "shaders/program_params.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <vector>

namespace df
{
class DebugRectRenderer : public dp::DebugRenderer
{
  using Base = dp::MeshObject;

public:
  DebugRectRenderer(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                    ref_ptr<gpu::ProgramParamsSetter> paramsSetter);

  void SetEnabled(bool enabled);

  bool IsEnabled() const override;
  void DrawRect(ref_ptr<dp::GraphicsContext> context, ScreenBase const & screen, m2::RectF const & rect,
                dp::Color const & color) override;
  void DrawArrow(ref_ptr<dp::GraphicsContext> context, ScreenBase const & screen,
                 dp::OverlayTree::DisplacementData const & data) override;

  void FinishRendering();

private:
  void SetArrow(ref_ptr<dp::GraphicsContext> context, m2::PointF const & arrowStart, m2::PointF const & arrowEnd,
                ScreenBase const & screen);
  void SetRect(ref_ptr<dp::GraphicsContext> context, m2::RectF const & rect, ScreenBase const & screen);

  std::vector<drape_ptr<dp::MeshObject>> m_rectMeshes;
  std::vector<drape_ptr<dp::MeshObject>> m_arrowMeshes;
  size_t m_currentRectMesh = 0;
  size_t m_currentArrowMesh = 0;
  ref_ptr<dp::GpuProgram> m_program;
  ref_ptr<gpu::ProgramParamsSetter> m_paramsSetter;
  dp::RenderState m_state;

  bool m_isEnabled = false;
};
}  // namespace df
