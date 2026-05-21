#pragma once

#include "drape_frontend/gui/ruler_helper.hpp"
#include "drape_frontend/gui/scale_fps_helper.hpp"
#include "drape_frontend/gui/shape.hpp"

#include "drape/drape_global.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
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

  // StringUtf8Multilang code of the system UI locale, used as the HarfBuzz `locl` hint when
  // shaping UI text (ruler, copyright, debug overlays). Set once at engine init from
  // languages::GetCurrentNorm() and remains kUnsupportedLanguageCode when the system locale is
  // not recognized -- ToHarfbuzzLanguage short-circuits negative codes to HB_LANGUAGE_INVALID.
  void SetUILang(int8_t lang) { m_uiLang = lang; }
  int8_t GetUILang() const { return m_uiLang; }

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
  int8_t m_uiLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  ScaleFpsHelper m_scaleFpsHelper;
};
}  // namespace gui
