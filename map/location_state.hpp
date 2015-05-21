#pragma once

#include "geometry/point2d.hpp"

#include "base/timer.hpp"

#include "routing/turns.hpp"

#include "platform/location.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/map.hpp"

class Framework;
class ScreenBase;

namespace location
{
  class GpsInfo;
  class CompassInfo;

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State
  {

  public:
//    void RouteBuilded();
//    void StartRouteFollow();
//    void StopRoutingMode();

//    /// @name User input notification block
//    //@{
//    void DragStarted();
//    void DragEnded();

//    void ScaleStarted();
//    void CorrectScalePoint(m2::PointD & pt) const;
//    void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const;
//    void ScaleEnded();

//    void Rotated();
//    //@}

//    void OnCompassTaped();

//    void OnSize();

  private:

    //m2::PointD const GetModeDefaultPixelBinding(Mode mode) const;
    //m2::PointD const GetRaFModeDefaultPxBind() const;

  private:
    uint16_t m_dragModeInfo = 0;
    uint16_t m_scaleModeInfo = 0;
  };
}
