#import "MWMSearchSuggestionCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

@interface MWMSearchSuggestionCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * separatorLeftOffset;

@end

@implementation MWMSearchSuggestionCell

- (void)awakeFromNib
{
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
}

- (NSDictionary *)selectedTitleAttributes
{
  return @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.bold16};
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.regular16};
}

+ (CGFloat)cellHeight
{
  return 44.0;
}

#pragma mark - Properties

- (void)setIsLastCell:(BOOL)isLastCell
{
  _isLastCell = isLastCell;
  self.separatorLeftOffset.constant = isLastCell ? 0.0 : 60.0;
}

@end
