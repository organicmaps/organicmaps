#import <CoreLocation/CoreLocation.h>
#import <XCTest/XCTest.h>

#import "MWMLocationManager.h"

@interface MWMLocationManager (Testing) <CLLocationManagerDelegate>
+ (MWMLocationManager *)manager;
- (instancetype)initManager;
- (void)setLocationManager:(CLLocationManager *)locationManager;
@end

@interface HeadingLossTestLocationManager : MWMLocationManager
@end

namespace
{
MWMLocationManager * g_testManager;
}  // namespace

@implementation HeadingLossTestLocationManager
+ (MWMLocationManager *)manager
{
  return g_testManager;
}

- (BOOL)start
{
  // Exercise the manager's start and stop state without requesting sensor updates.
  return YES;
}
@end

@interface HeadingLossTestHeading : CLHeading
@property(nonatomic) CLLocationDirection testAccuracy;
@property(nonatomic) CLLocationDirection testDirection;
@end

@implementation HeadingLossTestHeading
- (CLLocationDirection)headingAccuracy
{
  return self.testAccuracy;
}

- (CLLocationDirection)trueHeading
{
  return self.testDirection;
}

- (NSString *)description
{
  return [NSString stringWithFormat:@"Heading %.0f, accuracy %.0f", self.testDirection, self.testAccuracy];
}
@end

namespace
{
CLHeading * MakeHeading(CLLocationDirection direction, CLLocationDirection accuracy = 5)
{
  HeadingLossTestHeading * heading = [[HeadingLossTestHeading alloc] init];
  heading.testDirection = direction;
  heading.testAccuracy = accuracy;
  return heading;
}
}  // namespace

@interface HeadingLossTestObserver : NSObject <MWMLocationObserver>
@property(nonatomic, nullable) CLHeading * heading;
@property(nonatomic, nullable) CLLocation * location;
@property(nonatomic) NSUInteger headingUpdates;
@property(nonatomic) NSUInteger locationUpdates;
@end

@implementation HeadingLossTestObserver
- (void)onHeadingUpdate:(nullable CLHeading *)heading
{
  self.heading = heading;
  ++_headingUpdates;
}

- (void)onLocationUpdate:(CLLocation *)location
{
  self.location = location;
  ++_locationUpdates;
}
@end

@interface MWMLocationManagerTests : XCTestCase
@property(nonatomic) CLLocationManager * coreLocation;
@end

@implementation MWMLocationManagerTests
- (void)setUp
{
  [super setUp];
  g_testManager = [[HeadingLossTestLocationManager alloc] initManager];
  self.coreLocation = [[CLLocationManager alloc] init];
  [HeadingLossTestLocationManager start];
}

- (void)tearDown
{
  g_testManager = nil;
  [super tearDown];
}

- (void)drainMainQueue
{
  XCTestExpectation * ready = [self expectationWithDescription:@"Observer registration completed"];
  dispatch_async(dispatch_get_main_queue(), ^{ [ready fulfill]; });
  [self waitForExpectations:@[ready] timeout:2.0];
}

- (HeadingLossTestObserver *)registerObserver
{
  HeadingLossTestObserver * observer = [[HeadingLossTestObserver alloc] init];
  [HeadingLossTestLocationManager addObserver:observer];
  [self drainMainQueue];
  return observer;
}

- (void)deliverHeading:(CLHeading *)heading
{
  [g_testManager locationManager:self.coreLocation didUpdateHeading:heading];
}

- (void)deliverError:(CLError)code
{
  [g_testManager locationManager:self.coreLocation
                didFailWithError:[NSError errorWithDomain:kCLErrorDomain code:code userInfo:nil]];
}

- (void)testInvalidHeadingClearsAndReportsLossOnce
{
  CLHeading * valid = MakeHeading(0);
  [self deliverHeading:valid];
  HeadingLossTestObserver * observer = [self registerObserver];
  XCTAssertEqual(observer.heading, valid);

  [self deliverHeading:MakeHeading(0, -1)];

  XCTAssertNil(observer.heading);
  XCTAssertNil([HeadingLossTestLocationManager lastHeading]);

  [self deliverHeading:MakeHeading(90, -1)];
  [self deliverError:kCLErrorHeadingFailure];
  XCTAssertEqual(observer.headingUpdates, 2u);
}

- (void)testValidHeadingAfterLossReachesExistingObserver
{
  HeadingLossTestObserver * observer = [self registerObserver];
  [self deliverHeading:MakeHeading(0)];
  [self deliverHeading:MakeHeading(0, -1)];

  CLHeading * recovered = MakeHeading(90);
  [self deliverHeading:recovered];

  XCTAssertEqual(observer.heading, recovered);
  XCTAssertEqual([HeadingLossTestLocationManager lastHeading], recovered);
}

- (void)testRegistrationReplaysHeadingReceivedWhileQueuedOnlyToNewObserver
{
  HeadingLossTestObserver * first = [self registerObserver];
  HeadingLossTestObserver * second = [[HeadingLossTestObserver alloc] init];
  CLHeading * heading = MakeHeading(90);

  [HeadingLossTestLocationManager addObserver:second];
  [self deliverHeading:heading];
  [self drainMainQueue];

  XCTAssertEqual(first.heading, heading);
  XCTAssertEqual(first.headingUpdates, 2u);
  XCTAssertEqual(second.heading, heading);
  XCTAssertEqual(second.headingUpdates, 1u);
}

- (void)testHeadingFailureClearsLiveAndReplayedHeading
{
  CLHeading * valid = MakeHeading(0);
  [self deliverHeading:valid];
  HeadingLossTestObserver * first = [self registerObserver];
  XCTAssertEqual(first.heading, valid);

  [self deliverError:kCLErrorLocationUnknown];
  XCTAssertEqual(first.heading, valid);
  [self deliverError:kCLErrorHeadingFailure];

  XCTAssertNil(first.heading);
  XCTAssertNil([HeadingLossTestLocationManager lastHeading]);
  HeadingLossTestObserver * second = [self registerObserver];
  XCTAssertNil(second.heading);
  XCTAssertEqual(second.headingUpdates, 1u);
}

- (void)testStopClearsHeadingUntilServiceDeliversANewOne
{
  [g_testManager setLocationManager:self.coreLocation];
  CLHeading * valid = MakeHeading(0);
  [self deliverHeading:valid];
  HeadingLossTestObserver * first = [self registerObserver];
  XCTAssertEqual(first.heading, valid);

  [HeadingLossTestLocationManager stop];
  XCTAssertNil(first.heading);

  [HeadingLossTestLocationManager start];
  HeadingLossTestObserver * second = [self registerObserver];
  XCTAssertNil(second.heading);
  XCTAssertNil([HeadingLossTestLocationManager lastHeading]);

  CLHeading * recovered = MakeHeading(90);
  [self deliverHeading:recovered];
  XCTAssertEqual(first.heading, recovered);
  XCTAssertEqual(second.heading, recovered);
}

- (void)testLocationDenialClearsLiveAndReplayedHeading
{
  CLHeading * valid = MakeHeading(0);
  [self deliverHeading:valid];
  HeadingLossTestObserver * first = [self registerObserver];
  XCTAssertEqual(first.heading, valid);

  NSUserDefaults * defaults = NSUserDefaults.standardUserDefaults;
  NSString * const alertKey = @"kLocationAlertNeedShowKey";
  id const alertSetting = [defaults objectForKey:alertKey];
  [defaults setBool:NO forKey:alertKey];
  [self deliverError:kCLErrorDenied];
  [defaults setObject:alertSetting forKey:alertKey];

  XCTAssertTrue([HeadingLossTestLocationManager isLocationProhibited]);
  XCTAssertNil(first.heading);
  XCTAssertNil([HeadingLossTestLocationManager lastHeading]);
  XCTAssertNil([self registerObserver].heading);
}

- (void)testRegistrationReplaysLocationOnlyToNewObserver
{
  CLLocation * location = [[CLLocation alloc] initWithLatitude:52 longitude:13];
  [g_testManager locationManager:self.coreLocation didUpdateLocations:@[location]];

  HeadingLossTestObserver * first = [self registerObserver];
  XCTAssertEqual(first.locationUpdates, 1u);

  HeadingLossTestObserver * second = [self registerObserver];
  XCTAssertEqual(first.locationUpdates, 1u);
  XCTAssertEqual(second.locationUpdates, 1u);
  XCTAssertEqual(second.location.coordinate.latitude, location.coordinate.latitude);
  XCTAssertEqual(second.location.coordinate.longitude, location.coordinate.longitude);
}
@end
