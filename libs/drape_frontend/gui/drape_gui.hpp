#pragma once

#include "drape_frontend/gui/ruler_helper.hpp"
#include "drape_frontend/gui/scale_fps_helper.hpp"
#include "drape_frontend/gui/shape.hpp"

#include "drape/drape_global.hpp"

#include "geometry/rect2d.hpp"

#include <mutex>

class ScreenBase;

namespace gui
{

class DrapeGui
{
public:
  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();
  static dp::FontDecl GetGuiTextFont();

  void SetSurfaceSize(m2::PointF const & size);
  m2::PointF GetSurfaceSize() const;
  void SetVisibleViewport(m2::RectD const & rect);
  m2::RectD GetVisibleViewport() const;

  bool IsInUserAction() const { return m_inUserAction; }
  void SetInUserAction(bool isInUserAction) { m_inUserAction = isInUserAction; }

  bool IsCopyrightActive() const { return m_isCopyrightActive; }
  void DeactivateCopyright() { m_isCopyrightActive = false; }

  void ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler);
  void CallOnCompassTappedHandler();

  ScaleFpsHelper & GetScaleFpsHelper() { return m_scaleFpsHelper; }
  ScaleFpsHelper const & GetScaleFpsHelper() const { return m_scaleFpsHelper; }

private:
  RulerHelper m_rulerHelper;

  bool m_isCopyrightActive = true;

  Shape::TTapHandler m_onCompassTappedHandler;

  mutable std::mutex m_paramsMutex;
  m2::RectD m_visibleViewport;
  m2::PointF m_surfaceSize;

  bool m_inUserAction = false;
  ScaleFpsHelper m_scaleFpsHelper;
};
}  // namespace gui
