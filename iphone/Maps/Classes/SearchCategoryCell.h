
#import "SearchCell.h"

@interface SearchCategoryCell : SearchCell

@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) UIImageView * iconImageView;

+ (CGFloat)cellHeight;

@end
