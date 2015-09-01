
#import <UIKit/UIKit.h>
#import "DownloadIndicatorProtocol.h"
#import "NavigationController.h"
#import "MapsObservers.h"

#include "indexer/map_style.hpp"

@class MapViewController;
@class LocationManager;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate, UIAlertViewDelegate, ActiveMapsObserverProtocol, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
  NavigationController * m_navController;
  UIWindow * m_window;
  UIAlertView * m_loadingAlertView;
}

@property (nonatomic, weak) IBOutlet MapViewController * m_mapViewController;
@property (nonatomic, readonly) LocationManager * m_locationManager;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;

- (void)setMapStyle:(MapStyle)mapStyle;

@end
