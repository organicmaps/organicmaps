#import <UIKit/UIKit.h>

@class MapViewController;
@class GuideViewController;
@class SettingsManager;
@class UIWindow;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate>
{
  GuideViewController * m_guideViewController;
  SettingsManager * m_settingsManager;
}

@property (nonatomic, retain) IBOutlet UIWindow * window;
@property (nonatomic, retain) IBOutlet MapViewController * mapViewController;
@property (nonatomic, retain, readonly) GuideViewController * guideViewController;
@property (nonatomic, retain, readonly) SettingsManager * settingsManager;

+ (MapsAppDelegate *) theApp;

- (GuideViewController *)guideViewController;
- (SettingsManager *)settingsManager;
- (void)onSloynikEngineInitialized:(void *)pEngine;

@end
