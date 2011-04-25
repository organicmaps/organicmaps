#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

#include <boost/function.hpp>

namespace location
{
  /// after this period we cont position as "too old"
  static double const POSITION_TIMEOUT_SECONDS = 300.0;

  enum TLocationStatus
  {
    ENotSupported,    //!< GpsInfo fields are not valid with this value
    EDisabledByUser,  //!< GpsInfo fields are not valid with this value
    EAccurateMode,
    ERoughMode        //!< in this mode compass is turned off
  };

  /// @note always check m_status before using this structure
  class GpsInfo
  {
  public:
    TLocationStatus m_status;
    double m_timestamp;           //!< how many seconds ago the position was retrieved
    double m_latitude;            //!< degrees
    double m_longitude;           //!< degrees
    double m_horizontalAccuracy;  //!< metres
    double m_altitude;            //!< metres
    double m_verticalAccuracy;    //!< metres
    double m_course;              //!< positive degrees from the true North
    double m_speed;               //!< metres per second
  };

  /// @note always check m_status before using this structure
  class CompassInfo
  {
  public:
    double m_timestamp;           //!< how many seconds ago the heading was retrieved
    double m_magneticHeading;     //!< positive degrees from the magnetic North
    double m_trueHeading;         //!< positive degrees from the true North
    double m_accuracy;            //!< offset from magnetic to true North
    int m_x;
    int m_y;
    int m_z;
  };

  typedef boost::function<void (GpsInfo const &)> TGpsCallback;
  typedef boost::function<void (CompassInfo const &)> TCompassCallback;

  class LocationService
  {
    TGpsCallback m_gpsObserver;
    TCompassCallback m_compassObserver;

  protected:
    void NotifyGpsObserver(GpsInfo const & info)
    {
      if (m_gpsObserver)
        m_gpsObserver(info);
    }
    void NotifyCompassObserver(CompassInfo const & info)
    {
      if (m_compassObserver)
        m_compassObserver(info);
    }

  public:
    void SetGpsObserver(TGpsCallback observer)
    {
      m_gpsObserver = observer;
    }

    void SetCompassObserver(TCompassCallback observer)
    {
      m_compassObserver = observer;
    }

    /// to change active accuracy mode just call it again
    /// @note also enables compass if it's available
    virtual void StartUpdate(bool useAccurateMode) = 0;
    virtual void StopUpdate() = 0;
  };

}

extern "C" location::LocationService & GetLocationService();
