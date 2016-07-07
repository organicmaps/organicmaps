#import "MWMMapViewControlsManager.h"
#import "MWMNoMapsViewController.h"

@implementation MWMNoMapsViewController

- (IBAction)downloadMaps
{
  [[MWMMapViewControlsManager manager] actionDownloadMaps:mwm::DownloaderMode::Available];
}

@end
