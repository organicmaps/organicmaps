#pragma once

#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#ifdef BUILD_DESIGNER
#define RENDER_DEBUG_RECTS
#endif // BUILD_DESIGNER

namespace dp
{

class DebugRectRenderer
{
public:
  static DebugRectRenderer & Instance();
  void Init(ref_ptr<dp::GpuProgramManager> mng, int programId);
  void Destroy();

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  void DrawRect(ScreenBase const & screen, m2::RectF const & rect, dp::Color const & color) const;
  void DrawArrow(ScreenBase const & screen, OverlayTree::DisplacementData const & data) const;

private:
  DebugRectRenderer();
  ~DebugRectRenderer();

  uint32_t m_VAO;
  uint32_t m_vertexBuffer;
  ref_ptr<dp::GpuProgram> m_program;
  bool m_isEnabled;
};

} // namespace dp

