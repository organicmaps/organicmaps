
#import <UIKit/UIKit.h>
#import "BadgeView.h"

@interface BottomMenuCell : UITableViewCell

@property (nonatomic, readonly) UIImageView * iconImageView;
@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) BadgeView * badgeView;
@property (nonatomic, readonly) UIImageView * separator;

+ (CGFloat)cellHeight;

@end
