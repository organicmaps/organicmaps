
#import <UIKit/UIKit.h>
#include "../../geometry/point2d.hpp"

@class PlacePageInfoCell;
@protocol PlacePageInfoCellDelegate <NSObject>

- (void)infoCellDidPressCoordinates:(PlacePageInfoCell *)cell withGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer;
- (void)infoCellDidPressAddress:(PlacePageInfoCell *)cell withGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer;

@end

@interface PlacePageInfoCell : UITableViewCell

+ (CGFloat)cellHeightWithAddress:(NSString *)address viewWidth:(CGFloat)viewWidth;

- (void)setAddress:(NSString *)address pinPoint:(m2::PointD)point;

@property (nonatomic) UIImageView * separator;
@property (nonatomic, weak) id <PlacePageInfoCellDelegate> delegate;

@end
