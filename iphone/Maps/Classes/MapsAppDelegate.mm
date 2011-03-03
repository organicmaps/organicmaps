#import "MapsAppDelegate.hpp"
#import "MapViewController.hpp"

@implementation MapsAppDelegate

@synthesize window;
@synthesize mapViewController;

// here we're 
- (void) applicationWillTerminate: (UIApplication *) application
{
	[mapViewController OnTerminate];
}

- (void) applicationDidFinishLaunching: (UIApplication *) application
{
  // Add the tab bar controller's current view as a subview of the window
  [window addSubview:mapViewController.view];
  [window makeKeyAndVisible];
}

- (void) dealloc
{
  [mapViewController release];
  [window release];
  [super dealloc];
}

@end

