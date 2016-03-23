#import "MWMViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkDescriptionViewController : MWMViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager;

@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
