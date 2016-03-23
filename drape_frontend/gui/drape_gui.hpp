#pragma once

#include "skin.hpp"
#include "compass.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "drape/pointers.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"
#include "std/mutex.hpp"

class ScreenBase;

namespace gui
{

class RulerHelper;

class DrapeGui
{
public:
  using TLocalizeStringFn = function<string (string const &)>;

  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();

  static dp::FontDecl GetGuiTextFont();

  void SetLocalizator(TLocalizeStringFn const & fn);
  void Destroy();
  void SetSurfaceSize(m2::PointF const & size);
  m2::PointF GetSurfaceSize() const;

  string GetLocalizedString(string const & stringID) const;

  bool IsInUserAction() const { return m_inUserAction; }
  void SetInUserAction(bool isInUserAction) { m_inUserAction = isInUserAction; }

  bool IsCopyrightActive() const { return m_isCopyrightActive; }
  void DeactivateCopyright() { m_isCopyrightActive = false; }

  void ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler);
  void CallOnCompassTappedHandler();

private:
  DrapeGui();

  RulerHelper & GetRulerHelperImpl();

private:
  struct Impl;
  unique_ptr<Impl> m_impl;
  bool m_isCopyrightActive = true;

  Shape::TTapHandler m_onCompassTappedHandler;
  m2::PointF m_surfaceSize;
  mutable mutex m_surfaceSizeMutex;
  bool m_inUserAction = false;
};

} // namespace gui
