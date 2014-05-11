
#import <UIKit/UIKit.h>
#import "ContextViews.h"

@interface PlacePageEditCell : UITableViewCell

+ (CGFloat)cellHeightWithTextValue:(NSString *)text viewWidth:(CGFloat)viewWidth;

@property (nonatomic) UILabel * titleLabel;

@end
