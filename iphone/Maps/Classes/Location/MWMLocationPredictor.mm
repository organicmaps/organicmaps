#import "MWMLocationPredictor.h"

#include "Framework.h"

namespace
{
NSTimeInterval constexpr kPredictionIntervalInSeconds = 0.5;
NSUInteger constexpr kMaxPredictionCount = 20;
}  // namespace

@interface MWMLocationPredictor ()

@property (nonatomic) location::GpsInfo lastLocationInfo;
@property (nonatomic) BOOL isLastLocationInfoValid;
@property (nonatomic) BOOL isLastPositionModeValid;
@property (nonatomic) NSUInteger predictionsCount;
@property (copy, nonatomic) TPredictionBlock onPredictionBlock;

@end

@implementation MWMLocationPredictor

- (instancetype)initWithOnPredictionBlock:(TPredictionBlock)onPredictionBlock
{
  self = [super init];
  if (self)
    _onPredictionBlock = [onPredictionBlock copy];
  return self;
}

- (void)setMyPositionMode:(location::EMyPositionMode)mode
{
  self.isLastPositionModeValid = (mode == location::FollowAndRotate);
  [self restart];
}

- (void)reset:(location::GpsInfo const &)locationInfo
{
  self.isLastLocationInfoValid = (locationInfo.HasSpeed() && locationInfo.HasBearing());
  if (self.isLastLocationInfoValid)
    self.lastLocationInfo = locationInfo;

  [self restart];
}

- (BOOL)isActive
{
  return self.isLastLocationInfoValid && self.isLastPositionModeValid && self.predictionsCount < kMaxPredictionCount;
}

- (void)restart
{
  self.predictionsCount = 0;
  [self schedule];
}

- (void)schedule
{
  SEL const predict = @selector(predict);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:predict object:nil];
  [self performSelector:predict withObject:nil afterDelay:kPredictionIntervalInSeconds];
}

- (void)predict
{
  if (!self.isActive)
    return;

  self.predictionsCount++;

  location::GpsInfo info = self.lastLocationInfo;
  info.m_source = location::EPredictor;
  info.m_timestamp = [NSDate date].timeIntervalSince1970;
  Framework::PredictLocation(info.m_latitude, info.m_longitude, info.m_horizontalAccuracy,
                             info.m_bearing, info.m_speed,
                             info.m_timestamp - self.lastLocationInfo.m_timestamp);
  self.onPredictionBlock(info);
  [self schedule];
}

@end
