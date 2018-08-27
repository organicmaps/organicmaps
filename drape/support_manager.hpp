#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "base/macros.hpp"

namespace dp
{
extern char const * kSupportedAntialiasing;

class SupportManager
{
public:
  // This singleton must be available only from rendering threads.
  static SupportManager & Instance();

  // Initialization must be called only when graphics context is created.
  void Init(ref_ptr<GraphicsContext> context);

  bool IsSamsungGoogleNexus() const { return m_isSamsungGoogleNexus; }
  bool IsAdreno200Device() const { return m_isAdreno200; }
  bool IsTegraDevice() const { return m_isTegra; }
  int GetMaxLineWidth() const { return m_maxLineWidth; }
  bool IsAntialiasingEnabledByDefault() const { return m_isAntialiasingEnabledByDefault; }

private:
  SupportManager() = default;

  bool m_isSamsungGoogleNexus = false;
  bool m_isAdreno200 = false;
  bool m_isTegra = false;
  int m_maxLineWidth = 1;
  bool m_isAntialiasingEnabledByDefault = false;

  DISALLOW_COPY_AND_MOVE(SupportManager);
};
}  // namespace dp
