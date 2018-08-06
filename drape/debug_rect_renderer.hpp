#pragma once

#include "drape/gpu_program.hpp"
#include "drape/mesh_object.hpp"
#include "drape/overlay_tree.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <functional>

#ifdef BUILD_DESIGNER
#define RENDER_DEBUG_RECTS
#endif // BUILD_DESIGNER

namespace dp
{
class DebugRectRenderer: public dp::MeshObject
{
  using TBase = dp::MeshObject;
public:
  static DebugRectRenderer & Instance();

  using ParamsSetter = std::function<void(ref_ptr<dp::GpuProgram> program, dp::Color const & color)>;

  void Init(ref_ptr<dp::GpuProgram> program, ParamsSetter && paramsSetter);
  void Destroy();

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  void DrawRect(ScreenBase const & screen, m2::RectF const & rect, dp::Color const & color);
  void DrawArrow(ScreenBase const & screen, OverlayTree::DisplacementData const & data);

private:
  DebugRectRenderer();
  ~DebugRectRenderer() override;

  void SetArrow(m2::PointF const & arrowStart, m2::PointF const & arrowEnd, dp::Color const & arrowColor,
                ScreenBase const & screen);
  void SetRect(m2::RectF const & rect, ScreenBase const & screen);

  ParamsSetter m_paramsSetter;
  ref_ptr<dp::GpuProgram> m_program;
  bool m_isEnabled = false;
};
}  // namespace dp

