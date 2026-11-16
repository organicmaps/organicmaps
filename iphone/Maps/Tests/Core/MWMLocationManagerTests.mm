#import <CoreApi/ElevationProfileData.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <XCTest/XCTest.h>

#import "MWMLocationManager.h"

@interface MWMLocationManager (Testing) <CLLocationManagerDelegate>
+ (MWMLocationManager *)manager;
- (CLLocation *)lastLocationInfo;
@end

namespace
{
CLLocation * MakeLocation(double longitude, double accuracy, NSTimeInterval timestamp)
{
  return [[CLLocation alloc] initWithCoordinate:CLLocationCoordinate2DMake(0, longitude)
                                       altitude:10
                             horizontalAccuracy:accuracy
                               verticalAccuracy:5
                                         course:90
                                          speed:5
                                      timestamp:[NSDate dateWithTimeIntervalSince1970:timestamp]];
}
}  // namespace

// The tests drive the shared MWMLocationManager, so later tests in the same host start from its last fix.
@interface MWMLocationManagerTests : XCTestCase <MWMLocationObserver>
@property(nonatomic) NSMutableArray<CLLocation *> * positionUpdates;
@end

@implementation MWMLocationManagerTests

- (void)setUp
{
  [MWMFrameworkHelper discardTrackRecording];
  [self registerLocationObserver];
}

- (void)tearDown
{
  [MWMLocationManager removeObserver:self];
  [MWMFrameworkHelper discardTrackRecording];
  [super tearDown];
}

- (void)onLocationUpdate:(CLLocation *)location
{
  [self.positionUpdates addObject:location];
}

- (void)registerLocationObserver
{
  [MWMLocationManager addObserver:self];
  XCTestExpectation * observing = [self expectationWithDescription:@"Location observer registered"];
  dispatch_async(dispatch_get_main_queue(), ^{ [observing fulfill]; });
  [self waitForExpectations:@[observing] timeout:5];
  // Created after the registration, so the replay of a cached fix to the new observer is not counted.
  self.positionUpdates = [NSMutableArray array];
}

- (void)deliverLocations:(NSArray<CLLocation *> *)locations
{
  [[MWMLocationManager manager] locationManager:[[CLLocationManager alloc] init] didUpdateLocations:locations];
}

- (void)testRecordingKeepsValidLocationsAroundInvalidOnes
{
  CLLocation * latestValid = MakeLocation(0.001, 5, 1020);
  [MWMFrameworkHelper startTrackRecording];
  [self deliverLocations:@[
    MakeLocation(0.0005, -1, 990), MakeLocation(0, 5, 1000), MakeLocation(0.0005, -1, 1005),
    MakeLocation(0.0005, 5, 1010), latestValid, MakeLocation(0.002, -1, 1040)
  ]];
  [MWMFrameworkHelper stopTrackRecording];

  NSArray<ElevationHeightPoint *> * recorded = MWMFrameworkHelper.trackRecordingElevationInfo.points;
  XCTAssertEqual(recorded.count, 3u);
  XCTAssertEqualWithAccuracy(recorded.lastObject.distance, 111.2, 1.0);
  XCTAssertEqual(self.positionUpdates.count, 1u);
  XCTAssertEqualObjects(self.positionUpdates.lastObject.timestamp, latestValid.timestamp);
  XCTAssertEqual(self.positionUpdates.lastObject.coordinate.longitude, latestValid.coordinate.longitude);
}

- (void)testBatchWithoutValidLocationsIsIgnored
{
  CLLocation * cached = MakeLocation(0, 5, 990);
  [self deliverLocations:@[cached]];
  [self.positionUpdates removeAllObjects];

  [MWMFrameworkHelper startTrackRecording];
  [self deliverLocations:@[MakeLocation(0, -1, 1000), MakeLocation(0.001, -1, 1020)]];
  [MWMFrameworkHelper stopTrackRecording];

  XCTAssertNil(MWMFrameworkHelper.trackRecordingElevationInfo);
  XCTAssertEqual(self.positionUpdates.count, 0u);
  XCTAssertEqualObjects([MWMLocationManager manager].lastLocationInfo.timestamp, cached.timestamp);
}

@end
