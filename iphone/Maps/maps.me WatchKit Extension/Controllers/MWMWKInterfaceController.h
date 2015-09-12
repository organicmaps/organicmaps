#import "MWMWatchLocationTrackerDelegate.h"

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>

@interface MWMWKInterfaceController : WKInterfaceController <MWMWatchLocationTrackerDelegate>

@property (nonatomic, readonly) BOOL haveLocation;

- (void)checkLocationService;

@end
