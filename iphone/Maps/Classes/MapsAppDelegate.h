#import "DownloadIndicatorProtocol.h"
#import "MapsObservers.h"
#import "MWMAlertViewController.h"
#import "MWMNavigationController.h"

#include "indexer/map_style.hpp"

@class MapViewController;
@class LocationManager;

typedef NS_ENUM(NSUInteger, MWMRoutingPlaneMode)
{
  MWMRoutingPlaneModeNone,
  MWMRoutingPlaneModePlacePage,
  MWMRoutingPlaneModeSearchSource,
  MWMRoutingPlaneModeSearchDestination
};

@interface MapsAppDelegate : UIResponder<UIApplicationDelegate, UIAlertViewDelegate,
                                         ActiveMapsObserverProtocol, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
  UIBackgroundTaskIdentifier m_editorUploadBackgroundTask;
  UIAlertView * m_loadingAlertView;
}

@property (nonatomic) UIWindow * window;
@property (nonatomic) MWMRoutingPlaneMode routingPlaneMode;

@property (nonatomic, readonly) MapViewController * mapViewController;
@property (nonatomic, readonly) LocationManager * m_locationManager;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;
- (void)startMapStyleChecker;
- (void)stopMapStyleChecker;
- (void)showAlertIfRequired;
+ (void)setAutoNightModeOff:(BOOL)off;
+ (BOOL)isAutoNightMode;
+ (void)resetToDefaultMapStyle;
+ (void)changeMapStyleIfNedeed;

- (void)setMapStyle:(MapStyle)mapStyle;

@end
