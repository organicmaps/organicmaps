#import "LocationPredictor.h"
#import "Framework.h"

#include "../../../base/timer.hpp"
#include "../../../base/logging.cpp"

#include "../../../map/location_state.hpp"

namespace
{
  NSTimeInterval const PREDICTION_INTERVAL = 0.2; // in seconds
  int const MAX_PREDICTION_COUNT = 20;
}

@implementation LocationPredictor
{
  NSObject<LocationObserver> * m_observer;
  NSTimer * m_timer;
  
  location::GpsInfo m_lastGpsInfo;
  bool m_gpsInfoIsValid;
  
  int m_connectionSlot;
  bool m_generatePredictions;
  int m_predictionCount;
}

-(id)initWithObserver:(NSObject<LocationObserver> *)observer
{
  if ((self = [super init]))
  {
    m_observer = observer;
    m_timer = nil;
    m_gpsInfoIsValid = false;
    m_generatePredictions = false;
    m_predictionCount = 0;
    
    m_connectionSlot = GetFramework().GetLocationState()->AddStateModeListener([self](location::State::Mode mode)
    {
      m_generatePredictions = mode == location::State::RotateAndFollow;
      m_gpsInfoIsValid = mode < location::State::NotFollow ? false : m_gpsInfoIsValid;
      [self resetTimer];
    });
  }
  
  return self;
}

-(void)dealloc
{
  GetFramework().GetLocationState()->RemoveStateModeListener(m_connectionSlot);
}

-(void)reset:(location::GpsInfo const &)info
{
  m_gpsInfoIsValid = true;
  m_lastGpsInfo = info;
  [self resetTimer];
}

-(void)resetTimer
{
  m_predictionCount = 0;
  if (m_timer != nil)
  {
    [m_timer invalidate];
    m_timer = nil;
  }
  
  if (m_gpsInfoIsValid && m_generatePredictions)
  {
    m_timer = [NSTimer scheduledTimerWithTimeInterval:PREDICTION_INTERVAL
                                               target:self
                                             selector:@selector(timerFired)
                                             userInfo:nil
                                              repeats:YES];
  }
}

-(void)timerFired
{
  if (!(m_gpsInfoIsValid && m_generatePredictions))
    return;
  
  if (m_lastGpsInfo.HasBearing() && m_lastGpsInfo.HasSpeed() && m_predictionCount < MAX_PREDICTION_COUNT)
  {
    ++m_predictionCount;
    
    location::GpsInfo info = m_lastGpsInfo;
    info.m_timestamp = my::Timer::LocalTime();
    
    double offsetInM = info.m_speed * (info.m_timestamp - m_lastGpsInfo.m_timestamp);
    
    double angle = my::DegToRad(90.0 - info.m_bearing);
    m2::PointD mercatorPt = MercatorBounds::MetresToXY(info.m_longitude, info.m_latitude, info.m_horizontalAccuracy).Center();
    mercatorPt = MercatorBounds::GetSmPoint(mercatorPt, offsetInM * cos(angle), offsetInM * sin(angle));
    info.m_longitude = MercatorBounds::XToLon(mercatorPt.x);
    info.m_latitude = MercatorBounds::YToLat(mercatorPt.y);
    
    [m_observer onLocationUpdate:info];
  }
}

@end