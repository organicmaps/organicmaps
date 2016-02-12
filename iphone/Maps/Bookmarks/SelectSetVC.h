#import "MWMTableViewController.h"

@class MWMPlacePageViewManager;

@interface SelectSetVC : MWMTableViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager;

@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
