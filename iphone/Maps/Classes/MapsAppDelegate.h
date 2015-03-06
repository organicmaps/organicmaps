
#import <UIKit/UIKit.h>
#import "NavigationController.h"
#import "MapsObservers.h"

@class MapViewController;
@class LocationManager;

extern NSString * const MapsStatusChangedNotification;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate, UIAlertViewDelegate, ActiveMapsObserverProtocol>
{
  NSInteger m_standbyCounter;
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
  NavigationController * m_navController;
  UIWindow * m_window;
  UIAlertView * m_loadingAlertView;
}

@property (nonatomic, strong) IBOutlet MapViewController * m_mapViewController;
@property (nonatomic, readonly) LocationManager * m_locationManager;

+ (MapsAppDelegate *)theApp;
- (UIWindow *)window;

- (void)disableStandby;
- (void)enableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;

@end
