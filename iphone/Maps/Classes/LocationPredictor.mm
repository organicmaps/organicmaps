#import "LocationPredictor.h"

#include "Framework.h"

#include "base/timer.hpp"

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
  }
  
  return self;
}

-(void)setMode:(location::EMyPositionMode) mode
{
  m_generatePredictions = (mode == location::MODE_ROTATE_AND_FOLLOW);
  if (mode < location::MODE_NOT_FOLLOW)
    m_gpsInfoIsValid = false;

  [self resetTimer];
}

-(void)reset:(location::GpsInfo const &)info
{
  if (info.HasSpeed() && info.HasBearing())
  {
    m_gpsInfoIsValid = true;
    m_lastGpsInfo = info;
    m_lastGpsInfo.m_timestamp = my::Timer::LocalTime();
    m_lastGpsInfo.m_source = location::EPredictor;
  }
  else
    m_gpsInfoIsValid = false;

  [self resetTimer];
}

-(bool)isPredict
{
  return m_gpsInfoIsValid && m_generatePredictions;
}

-(void)resetTimer
{
  m_predictionCount = 0;

  if (m_timer != nil)
  {
    [m_timer invalidate];
    m_timer = nil;
  }
  
  if ([self isPredict])
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
  if (![self isPredict])
    return;
  
  if (m_predictionCount < MAX_PREDICTION_COUNT)
  {
    ++m_predictionCount;

    location::GpsInfo info = m_lastGpsInfo;
    info.m_timestamp = my::Timer::LocalTime();
    ::Framework::PredictLocation(info.m_latitude, info.m_longitude, info.m_horizontalAccuracy, info.m_bearing,
                                 info.m_speed, info.m_timestamp - m_lastGpsInfo.m_timestamp);
    
    [m_observer onLocationUpdate:info];
  }
}

@end
