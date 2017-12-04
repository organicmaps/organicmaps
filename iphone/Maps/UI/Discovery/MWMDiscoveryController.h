#import "MWMViewController.h"

typedef NS_ENUM(NSUInteger, MWMDiscoveryMode)
{
  MWMDiscoveryModeOnline,
  MWMDiscoveryModeOffline
};

@interface MWMDiscoveryController : MWMViewController

+ (instancetype)instance;
- (void)setMode:(MWMDiscoveryMode)mode;

@end
