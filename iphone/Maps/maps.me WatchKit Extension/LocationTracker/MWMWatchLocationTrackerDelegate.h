#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@protocol MWMWatchLocationTrackerDelegate <NSObject>

@optional
- (void)onChangeLocation:(CLLocation *)location;
- (void)locationTrackingFailedWithError:(NSError *)error;
- (void)didChangeAuthorizationStatus:(BOOL)haveAuthorization;

@end
