#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

#include <boost/function.hpp>

namespace location
{
  enum TLocationStatus
  {
    ENotSupported,    //!< GpsInfo fields are not valid with this value
    EDisabledByUser,  //!< GpsInfo fields are not valid with this value
    EAccurateMode,
    ERoughMode        //!< in this mode compass is turned off
  };

  /// @note always check m_status before using this structure
  struct GpsInfo
  {
    TLocationStatus m_status;
    double m_timestamp;           //!< seconds from 01/01/1970
    double m_latitude;            //!< degrees  @TODO mercator
    double m_longitude;           //!< degrees  @TODO mercator
    double m_horizontalAccuracy;  //!< metres
    double m_altitude;            //!< metres
    double m_verticalAccuracy;    //!< metres
    double m_course;              //!< positive degrees from the true North
    double m_speed;               //!< metres per second
  };

  struct CompassInfo
  {
    double m_timestamp;           //!< seconds from 01/01/1970
    double m_magneticHeading;     //!< positive degrees from the magnetic North
    double m_trueHeading;         //!< positive degrees from the true North
    double m_accuracy;            //!< offset from magnetic to true North
    int m_x;
    int m_y;
    int m_z;
  };

  typedef boost::function1<void, GpsInfo const &> TGpsCallback;
  typedef boost::function1<void, CompassInfo const &> TCompassCallback;

  class LocationService
  {
    typedef vector<TGpsCallback> GpsObserversT;
    GpsObserversT m_gpsObservers;
    typedef vector<TCompassCallback> CompassObserversT;
    CompassObserversT m_compassObservers;

  protected:
    void NotifySubscribers(GpsInfo const & info);
    void NotifySubscribers(CompassInfo const & info);

  public:
    /// @note unsubscribe doesn't work with boost::function
    void SubscribeToGpsUpdates(TGpsCallback observer);
    void SubscribeToCompassUpdates(TCompassCallback observer);
//    void Unsubscribe(TGpsCallback observer);
//    void Unsubscribe(TCompassCallback observer);

    /// to change active accuracy mode just call it again
    /// @param useAccurateMode if true also enables compass if it's available
    virtual void StartUpdate(bool useAccurateMode) = 0;
    virtual void StopUpdate() = 0;
  };

}

extern "C" location::LocationService & GetLocationService();
