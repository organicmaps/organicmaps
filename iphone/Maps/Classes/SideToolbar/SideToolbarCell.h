
#import <UIKit/UIKit.h>

@interface SideToolbarCell : UITableViewCell

@property (nonatomic) UIImageView * iconImageView;
@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) BOOL disabled;

+ (CGFloat)cellHeight;

@end
