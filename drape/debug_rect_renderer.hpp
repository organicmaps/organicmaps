#pragma once

#include "drape/drape_diagnostics.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

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

#ifdef COLLECT_DISPLACEMENT_INFO
  void DrawArrow(ScreenBase const & screen, OverlayTree::DisplacementData const & data) const;
#endif

private:
  DebugRectRenderer();
  ~DebugRectRenderer();

  int m_VAO;
  int m_vertexBuffer;
  ref_ptr<dp::GpuProgram> m_program;
  bool m_isEnabled;
};

} // namespace dp

