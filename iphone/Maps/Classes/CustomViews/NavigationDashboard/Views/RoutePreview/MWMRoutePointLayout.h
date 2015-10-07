#import <UIKit/UIKit.h>

@interface MWMRoutePointLayout : UICollectionViewFlowLayout

@property (nonatomic) BOOL isNeedToInitialLayout;
@property (weak, nonatomic) IBOutlet UIView * parentView;

@end
