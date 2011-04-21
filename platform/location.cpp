#include "location.hpp"

#include "../std/algorithm.hpp"

#include <boost/bind.hpp>

namespace location
{
  void LocationService::NotifySubscribers(GpsInfo const & info)
  {
    for (GpsObserversT::iterator it = m_gpsObservers.begin();
         it != m_gpsObservers.end(); ++it)
      (*it)(info);
  }

  void LocationService::NotifySubscribers(CompassInfo const & info)
  {
    for (CompassObserversT::iterator it = m_compassObservers.begin();
         it != m_compassObservers.end(); ++it)
      (*it)(info);
  }

  void LocationService::SubscribeToGpsUpdates(TGpsCallback observer)
  {
//    if (std::find(m_gpsObservers.begin(), m_gpsObservers.end(), boost::bind(&observer))
//        == m_gpsObservers.end())
      m_gpsObservers.push_back(observer);
  }

  void LocationService::SubscribeToCompassUpdates(TCompassCallback observer)
  {
//    if (std::find(m_compassObservers.begin(), m_compassObservers.end(), observer)
//        == m_compassObservers.end())
      m_compassObservers.push_back(observer);
  }

//  void LocationService::Unsubscribe(TGpsCallback observer)
//  {
//    GpsObserversT::iterator found =
//        std::find(m_gpsObservers.begin(), m_gpsObservers.end(), observer);
//    if (found != m_gpsObservers.end())
//      m_gpsObservers.erase(found);
//  }

//  void LocationService::Unsubscribe(TCompassCallback observer)
//  {
//    CompassObserversT::iterator found =
//        std::find(m_compassObservers.begin(), m_compassObservers.end(), observer);
//    if (found != m_compassObservers.end())
//      m_compassObservers.erase(found);
//  }
}
