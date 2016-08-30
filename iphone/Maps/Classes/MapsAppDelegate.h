#import "DownloadIndicatorProtocol.h"
#import "MWMNavigationController.h"

#include "indexer/map_style.hpp"

#include "storage/index.hpp"

@class MapViewController;
@class LocationManager;

typedef NS_ENUM(NSUInteger, MWMRoutingPlaneMode) {
  MWMRoutingPlaneModeNone,
  MWMRoutingPlaneModePlacePage,
  MWMRoutingPlaneModeSearchSource,
  MWMRoutingPlaneModeSearchDestination
};

@interface MapsAppDelegate
    : UIResponder<UIApplicationDelegate, UIAlertViewDelegate, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
  UIBackgroundTaskIdentifier m_editorUploadBackgroundTask;
  UIAlertView * m_loadingAlertView;
}

@property(nonatomic) UIWindow * window;
@property(nonatomic) MWMRoutingPlaneMode routingPlaneMode;

@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (BOOL)hasApiURL;

+ (void)initPushNotificationsWithLaunchOptions:(NSDictionary *)launchOptions;

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
+ (void)resetToDefaultMapStyle;
+ (void)changeMapStyleIfNedeed;

- (void)setMapStyle:(MapStyle)mapStyle;

@end
