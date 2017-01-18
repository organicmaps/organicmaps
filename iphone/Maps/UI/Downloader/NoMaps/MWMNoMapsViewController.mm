#import "MWMNoMapsViewController.h"
#import "MWMMapViewControlsManager.h"
#import "UIViewController+Navigation.h"
#import "SwiftBridge.h"

@implementation MWMNoMapsViewController

+ (MWMNoMapsViewController *)controller
{
  auto storyboard = [UIStoryboard instance:MWMStoryboardMain];
  return [storyboard instantiateViewControllerWithIdentifier:[self className]];
}

- (IBAction)downloadMaps
{
  [[MWMMapViewControlsManager manager] actionDownloadMaps:mwm::DownloaderMode::Available];
}

@end
