#import "MWMSearchFilterViewController.h"
#import "MWMTypes.h"
#import "MWMHotelParams.h"

@class MWMSearchHotelsFilterViewController;

@protocol MWMSearchHotelsFilterViewControllerDelegate<NSObject>

- (void)hotelsFilterViewController:(MWMSearchHotelsFilterViewController *)viewController
                   didSelectParams:(MWMHotelParams *)params;
- (void)hotelsFilterViewControllerDidCancel:(MWMSearchHotelsFilterViewController *)viewController;
  
@end

@interface MWMSearchHotelsFilterViewController : MWMSearchFilterViewController

- (void)applyParams:(MWMHotelParams *)params;

@property (nonatomic, weak) id<MWMSearchHotelsFilterViewControllerDelegate> delegate;

@end
