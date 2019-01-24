#import "MWMSearchManager.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "MWMTypes.h"
#import "MWMHotelParams.h"

@interface MWMSearchManager (Filter)<MWMSearchHotelsFilterViewControllerDelegate>

- (void)clearFilter;
- (void)showHotelFilterWithParams:(MWMHotelParams *)params
                 onFinishCallback:(MWMVoidBlock)callback;

@end
