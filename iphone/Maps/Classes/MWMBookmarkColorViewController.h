#import "MWMTableViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkColorViewController : MWMTableViewController

@property (weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
