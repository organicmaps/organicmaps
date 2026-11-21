#import <XCTest/XCTest.h>

#import "EAGLView.h"

// Exercise the renderer's completion without creating a second Drape engine in the test host.
@interface EAGLView (Testing)
- (void)didCreateDrapeEngine;
@end

@interface MWMMapReadinessTests : XCTestCase
@end

@implementation MWMMapReadinessTests

- (void)testPendingActionsRunInOrderAndLateActionsRunImmediately
{
  EAGLView * map = [[EAGLView alloc] initWithFrame:CGRectZero];
  NSMutableArray<NSString *> * events = [NSMutableArray array];
  [map whenDrapeEngineCreated:^{ [events addObject:@"restore"]; }];
  [map whenDrapeEngineCreated:^{ [events addObject:@"search"]; }];
  XCTAssertEqual(events.count, 0u);

  [map didCreateDrapeEngine];
  XCTAssertEqualObjects(events, (@[@"restore", @"search"]));
  [map whenDrapeEngineCreated:^{ [events addObject:@"preview"]; }];
  XCTAssertEqualObjects(events, (@[@"restore", @"search", @"preview"]));
}

@end
