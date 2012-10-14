#import <UIKit/UIKit.h>

@class MapViewController;
@class SettingsManager;
@class LocationManager;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate>
{
  SettingsManager * m_settingsManager;
  NSInteger m_standbyCounter;
  NSInteger m_activeDownloadsCounter;
  UINavigationController * m_navController;
  UIWindow * m_window;
}

@property (nonatomic, retain) IBOutlet MapViewController * m_mapViewController;
@property (nonatomic, readonly) LocationManager * m_locationManager;

+ (MapsAppDelegate *)theApp;

- (SettingsManager *)settingsManager;

- (void)disableStandby;
- (void)enableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

@end
