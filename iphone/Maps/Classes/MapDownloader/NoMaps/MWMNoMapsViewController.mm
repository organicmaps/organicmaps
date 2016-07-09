#import "MWMNoMapsViewController.h"
#import "MWMMapViewControlsManager.h"

@implementation MWMNoMapsViewController

- (IBAction)downloadMaps
{
  [[MWMMapViewControlsManager manager] actionDownloadMaps:mwm::DownloaderMode::Available];
}

@end
