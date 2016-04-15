#import "MWMNoMapsViewController.h"

@implementation MWMNoMapsViewController

- (IBAction)downloadMaps
{
  [self.delegate handleDownloadMapsAction];
}

@end
