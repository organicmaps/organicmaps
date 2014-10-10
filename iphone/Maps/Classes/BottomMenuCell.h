
#import <UIKit/UIKit.h>
#import "BadgeView.h"

@interface BottomMenuCell : UITableViewCell

@property (nonatomic, readonly) UIImageView * iconImageView;
@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) BadgeView * badgeView;

+ (CGFloat)cellHeight;

@end
