
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, SearchSuggestCellPosition)
{
  SearchSuggestCellPositionMiddle,
  SearchSuggestCellPositionBottom
};

@interface SearchSuggestCell : UITableViewCell

@property (nonatomic) SearchSuggestCellPosition position;
@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) UIImageView * iconImageView;

+ (CGFloat)cellHeightWithPosition:(SearchSuggestCellPosition)position;

@end
