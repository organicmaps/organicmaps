#import "MWMNoMapsViewController.h"
#import "MWMMapViewControlsManager.h"
#import "UIViewController+Navigation.h"

@implementation MWMNoMapsViewController

+ (MWMNoMapsViewController *)controller
{
  return
      [[UIViewController mainStoryboard] instantiateViewControllerWithIdentifier:[self className]];
}

- (IBAction)downloadMaps
{
  [[MWMMapViewControlsManager manager] actionDownloadMaps:mwm::DownloaderMode::Available];
}

@end
