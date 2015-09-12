#import <UIKit/UIKit.h>
#import "ViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkDescriptionViewController : ViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager;

@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
