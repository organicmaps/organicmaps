#import "MWMViewController.h"

@protocol MWMNoMapsViewControllerProtocol <NSObject>

- (void)handleDownloadMapsAction;

@end

@interface MWMNoMapsViewController : MWMViewController

@property (nullable, weak, nonatomic) id<MWMNoMapsViewControllerProtocol> delegate;

@end
