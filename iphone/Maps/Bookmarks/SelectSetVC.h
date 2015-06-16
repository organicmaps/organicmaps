
#import <UIKit/UIKit.h>
#import "TableViewController.h"

@class MWMPlacePageViewManager;

@interface SelectSetVC : TableViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager;

@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
