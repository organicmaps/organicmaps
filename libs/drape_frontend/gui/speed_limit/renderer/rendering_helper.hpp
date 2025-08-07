#pragma once

#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

namespace gui::speed_limit::renderer
{
class RenderingHelper
{
public:
  explicit RenderingHelper(SpeedLimit const & speedLimit);

  m2::PointF GetPosition() const;

  bool IsVisible() const;
  bool IsBackgroundUpdateNeeded() const;
  bool IsTextUpdateNeeded() const;

  void UpdateBackgroundParams(gpu::GuiProgramParams & params, ref_ptr<dp::TextureManager> tex) const;

private:
  SpeedLimit const & m_speedLimit;
  mutable bool m_needsBackgroundUpdate = true;
  mutable bool m_needsTextUpdate = true;
};

using RenderingHelperPtr = std::shared_ptr<RenderingHelper const>;

}  // namespace gui::speed_limit::renderer
