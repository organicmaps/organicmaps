#import "MWMAuthorizationCommon.h"
#import "MWMNavigationController.h"

@class MapViewController;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder <UIApplicationDelegate>

// The connected main window, or nil before MainSceneDelegate attaches it (e.g. a CarPlay-first
// cold launch, where only the CarPlay scene exists).
@property(nonatomic, strong, nullable) UIWindow * window;

// The Main storyboard's root navigation controller. Lazily instantiated so a single shared
// MapViewController exists even before the phone window scene connects.
@property(nonatomic, readonly) UINavigationController * mainNavigationController;
@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;
- (void)completeOAuth2AuthorizationWithCompletion:(void (^)(MWMOAuth2Result result))completion
    NS_SWIFT_NAME(completeOAuth2Authorization(completion:));

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

// User activities and quick actions are delivered to MainSceneDelegate; their handling lives here
// because it needs the app-wide search and map plumbing.
- (BOOL)handleUserActivity:(NSUserActivity *)userActivity NS_SWIFT_NAME(handleUserActivity(_:));
- (void)handleShortcutItem:(UIApplicationShortcutItem *)shortcutItem
         completionHandler:(void (^)(BOOL))completionHandler NS_SWIFT_NAME(handleShortcutItem(_:completionHandler:));

- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

@end

NS_ASSUME_NONNULL_END
