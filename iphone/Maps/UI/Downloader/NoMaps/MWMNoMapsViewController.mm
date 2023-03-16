#import "MWMNoMapsViewController.h"
#import "MWMMapDownloaderMode.h"
#import "MWMMapViewControlsManager.h"
#import "SwiftBridge.h"

@implementation MWMNoMapsViewController

+ (MWMNoMapsViewController *)controller
{
  auto storyboard = [UIStoryboard instance:MWMStoryboardMain];
  return [storyboard instantiateViewControllerWithIdentifier:[self className]];
}

- (IBAction)downloadMaps
{
  [[MWMMapViewControlsManager manager] actionDownloadMaps:MWMMapDownloaderModeAvailable];
}

@end
