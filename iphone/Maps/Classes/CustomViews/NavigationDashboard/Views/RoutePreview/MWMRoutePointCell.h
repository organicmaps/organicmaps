#import <UIKit/UIKit.h>

@class MWMRoutePointCell;

@protocol MWMRoutePointCellDelegate <NSObject>

@required
- (void)didPan:(UIPanGestureRecognizer *)pan cell:(MWMRoutePointCell *)cell;

@end

@interface MWMRoutePointCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UITextField * title;
@property (weak, nonatomic) IBOutlet UILabel * number;
@property (weak, nonatomic) id<MWMRoutePointCellDelegate> delegate;

@end
