#import <CoreApi/MWMMapOverlayManager.h>
#import <XCTest/XCTest.h>

#import "MWMTrafficButtonViewController.h"
#import "MapViewController.h"

@protocol ToastTesting
+ (void)hideAll;
@end

@interface MWMTrafficButtonViewController (Testing)
- (void)handleIsolinesState:(MWMMapOverlayIsolinesState)state;
- (void)onMapOverlayUpdated;
@end

@interface MWMTrafficButtonViewControllerTests : XCTestCase
@property(nonatomic) MapViewController * mapController;
@property(nonatomic) UINavigationController * navigationController;
@property(nonatomic) NSArray<UIViewController *> * originalControllers;
@property(nonatomic) MWMTrafficButtonViewController * button;
@property(nonatomic) NSNumber * originalHandledState;
@property(nonatomic) BOOL originalIsolinesEnabled;
@end

@implementation MWMTrafficButtonViewControllerTests

- (void)setUp
{
  [super setUp];
  XCTAssertTrue(NSThread.isMainThread);
  self.mapController = MapViewController.sharedController;
  self.navigationController = self.mapController.navigationController;
  XCTAssertNotNil(self.navigationController);
  self.originalControllers = self.navigationController.viewControllers;
  [self.navigationController setViewControllers:@[self.mapController] animated:NO];
  self.button = MWMTrafficButtonViewController.controller;
  self.originalHandledState = [self.button valueForKey:@"lastHandledIsolinesState"];
  self.originalIsolinesEnabled = MWMMapOverlayManager.isoLinesEnabled;
  [MWMMapOverlayManager setIsoLinesEnabled:NO];
  [self.button setValue:@(MWMMapOverlayIsolinesStateDisabled) forKey:@"lastHandledIsolinesState"];
}

- (void)tearDown
{
  [MWMMapOverlayManager setIsoLinesEnabled:self.originalIsolinesEnabled];
  [self.navigationController setViewControllers:self.originalControllers animated:NO];
  [self.button setValue:self.originalHandledState forKey:@"lastHandledIsolinesState"];
  Class<ToastTesting> toast = (Class<ToastTesting>)NSClassFromString(@"Organic_Maps__Debug_.Toast");
  XCTAssertNotNil(toast);
  [toast hideAll];
  self.button = nil;
  self.originalControllers = nil;
  self.navigationController = nil;
  self.mapController = nil;
  [super tearDown];
}

- (void)openDownloader
{
  UIViewController * downloader = [[UIStoryboard storyboardWithName:@"Main" bundle:nil]
      instantiateViewControllerWithIdentifier:@"DownloadMapsViewController"];
  [self.navigationController pushViewController:downloader animated:NO];
  NSPredicate * hidden = [NSPredicate predicateWithBlock:^BOOL(id, NSDictionary *) {
    return self.navigationController.visibleViewController == downloader && self.mapController.view.window == nil;
  }];
  XCTestExpectation * finished = [self expectationForPredicate:hidden evaluatedWithObject:nil handler:nil];
  [self waitForExpectations:@[finished] timeout:2];
}

- (void)closeDownloader
{
  [self.navigationController popViewControllerAnimated:NO];
  NSPredicate * visible = [NSPredicate predicateWithBlock:^BOOL(id, NSDictionary *) {
    return self.navigationController.visibleViewController == self.mapController &&
           self.mapController.view.window != nil;
  }];
  XCTestExpectation * finished = [self expectationForPredicate:visible evaluatedWithObject:nil handler:nil];
  [self waitForExpectations:@[finished] timeout:2];
}

- (void)testTerrainHintsStayPendingWhileDownloaderCoversMap
{
  [self openDownloader];
  [self.button handleIsolinesState:MWMMapOverlayIsolinesStateNoData];
  [self.button handleIsolinesState:MWMMapOverlayIsolinesStateEnabled];
  XCTAssertEqualObjects([self.button valueForKey:@"lastHandledIsolinesState"], @(MWMMapOverlayIsolinesStateDisabled));

  [self closeDownloader];
  [self.button handleIsolinesState:MWMMapOverlayIsolinesStateNoData];
  XCTAssertEqualObjects([self.button valueForKey:@"lastHandledIsolinesState"], @(MWMMapOverlayIsolinesStateNoData));
}

- (void)testReturningToMapHandlesOverlayStateChangedInDownloader
{
  [self openDownloader];
  [MWMMapOverlayManager setIsoLinesEnabled:YES];
  [self.button onMapOverlayUpdated];
  XCTestExpectation * notified = [self expectationWithDescription:@"Hidden overlay notifications drained"];
  dispatch_async(dispatch_get_main_queue(), ^{ [notified fulfill]; });
  [self waitForExpectations:@[notified] timeout:2];
  XCTAssertEqualObjects([self.button valueForKey:@"lastHandledIsolinesState"], @(MWMMapOverlayIsolinesStateDisabled));

  [self closeDownloader];
  NSPredicate * handled = [NSPredicate predicateWithBlock:^BOOL(id, NSDictionary *) {
    return [[self.button valueForKey:@"lastHandledIsolinesState"] unsignedIntegerValue] ==
           MWMMapOverlayManager.isolinesState;
  }];
  XCTestExpectation * finished = [self expectationForPredicate:handled evaluatedWithObject:nil handler:nil];
  [self waitForExpectations:@[finished] timeout:2];
  XCTAssertEqualObjects([self.button valueForKey:@"lastHandledIsolinesState"], @(MWMMapOverlayManager.isolinesState));
}

@end
