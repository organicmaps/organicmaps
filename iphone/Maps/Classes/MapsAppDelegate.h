#import <UIKit/UIKit.h>

@class MapViewController;
@class UIWindow;

@interface MapsAppDelegate : NSObject <UIApplicationDelegate>
{
    UIWindow * window;
    MapViewController * mapViewController;
}

@property (nonatomic, retain) IBOutlet UIWindow * window;
@property (nonatomic, retain) IBOutlet MapViewController * mapViewController;

@end
