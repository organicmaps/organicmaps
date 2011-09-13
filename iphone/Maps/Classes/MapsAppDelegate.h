#import <UIKit/UIKit.h>

@class MapViewController;
@class SettingsManager;
@class Preferences;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate>
{
  UINavigationController * m_navigationController;
  UIWindow * m_window;
  MapViewController * m_mapViewController;
  SettingsManager * m_settingsManager;
  NSInteger m_standbyCounter;

  Preferences * m_preferences;
}

@property (nonatomic, retain) IBOutlet UINavigationController * m_navigationController;
@property (nonatomic, retain) IBOutlet UIWindow * m_window;
@property (nonatomic, retain) IBOutlet MapViewController * m_mapViewController;

+ (MapsAppDelegate *) theApp;

- (SettingsManager *)settingsManager;
- (void)disableStandby;
- (void)enableStandby;

@end
