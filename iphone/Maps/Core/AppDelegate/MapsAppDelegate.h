#import "DownloadIndicatorProtocol.h"
#import "MWMNavigationController.h"

@class MapViewController;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder <UIApplicationDelegate, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
}

@property(nonatomic) UIWindow * window;

// The connected main window, or nil before MainSceneDelegate attaches it (e.g. a CarPlay-first
// cold launch, where only the CarPlay scene exists). Use this instead of `window` when the phone
// scene may not have connected yet.
@property(nonatomic, readonly, nullable) UIWindow * connectedWindow;

// The Main storyboard's root navigation controller. Lazily instantiated so a single shared
// MapViewController exists even before the phone window scene connects.
@property(nonatomic, readonly) UINavigationController * mainNavigationController;
@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;
- (void)completeOAuth2Authorization;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

@end

NS_ASSUME_NONNULL_END
