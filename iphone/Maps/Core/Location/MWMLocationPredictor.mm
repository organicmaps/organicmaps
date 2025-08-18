#import "MWMLocationPredictor.h"

#include <CoreApi/Framework.h>

namespace
{
NSTimeInterval constexpr kPredictionIntervalInSeconds = 0.5;
NSUInteger constexpr kMaxPredictionCount = 20;
}  // namespace

@interface MWMLocationPredictor ()

@property(copy, nonatomic) CLLocation * lastLocation;
@property(nonatomic) BOOL isLastLocationValid;
@property(nonatomic) BOOL isLastPositionModeValid;
@property(nonatomic) NSUInteger predictionsCount;
@property(copy, nonatomic) TPredictionBlock onPredictionBlock;

@end

@implementation MWMLocationPredictor

- (instancetype)initWithOnPredictionBlock:(TPredictionBlock)onPredictionBlock
{
  self = [super init];
  if (self)
    _onPredictionBlock = [onPredictionBlock copy];
  return self;
}

- (void)setMyPositionMode:(MWMMyPositionMode)mode
{
  self.isLastPositionModeValid = (mode == MWMMyPositionModeFollowAndRotate);
  [self restart];
}

- (void)reset:(CLLocation *)location
{
  self.isLastLocationValid = (location.speed >= 0.0 && location.course >= 0.0);
  if (self.isLastLocationValid)
    self.lastLocation = location;

  [self restart];
}

- (BOOL)isActive
{
  return self.isLastLocationValid && self.isLastPositionModeValid && self.predictionsCount < kMaxPredictionCount;
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

  CLLocation * l = self.lastLocation;
  CLLocationCoordinate2D coordinate = l.coordinate;
  CLLocationDistance altitude = l.altitude;
  CLLocationAccuracy hAccuracy = l.horizontalAccuracy;
  CLLocationAccuracy vAccuracy = l.verticalAccuracy;
  CLLocationDirection course = l.course;
  CLLocationSpeed speed = l.speed;
  NSDate * timestamp = [NSDate date];
  Framework::PredictLocation(coordinate.latitude, coordinate.longitude, hAccuracy, course, speed,
                             timestamp.timeIntervalSince1970 - l.timestamp.timeIntervalSince1970);
  CLLocation * location = [[CLLocation alloc] initWithCoordinate:coordinate
                                                        altitude:altitude
                                              horizontalAccuracy:hAccuracy
                                                verticalAccuracy:vAccuracy
                                                          course:course
                                                           speed:speed
                                                       timestamp:timestamp];
  self.onPredictionBlock(location);
  [self schedule];
}

@end
