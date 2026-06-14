#import "MWMNavigationController.h"

@class MapViewController;
@class MWMCarPlayService;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder <UIApplicationDelegate>

@property(nonatomic) UIWindow * window;

@property(nonatomic, readonly) MWMCarPlayService * carplayService;
@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;
- (void)completeOAuth2Authorization;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

@end

NS_ASSUME_NONNULL_END
