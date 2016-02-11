#import "TableViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkColorViewController : TableViewController

@property (weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
