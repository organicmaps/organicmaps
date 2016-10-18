#import "MWMTableViewController.h"

@class MWMPlacePageViewManager;
@class MWMPlacePageData;

@interface MWMEditBookmarkController : MWMTableViewController

@property(weak, nonatomic) MWMPlacePageData * data;
@property (nonatomic) MWMPlacePageViewManager * manager;

@end
