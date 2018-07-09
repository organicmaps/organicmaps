#pragma once

#include "skin.hpp"
#include "compass.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "drape/pointers.hpp"

#include <functional>
#include <memory>
#include <mutex>

class ScreenBase;

namespace gui
{
class RulerHelper;

class DrapeGui
{
public:
  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();

  static dp::FontDecl GetGuiTextFont();

  void Destroy();
  void SetSurfaceSize(m2::PointF const & size);
  m2::PointF GetSurfaceSize() const;

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
  std::unique_ptr<Impl> m_impl;
  bool m_isCopyrightActive = true;

  Shape::TTapHandler m_onCompassTappedHandler;
  m2::PointF m_surfaceSize;
  mutable std::mutex m_surfaceSizeMutex;
  bool m_inUserAction = false;
};
}  // namespace gui
