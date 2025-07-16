#import "MWMSearchSuggestionCell.h"

@interface MWMSearchSuggestionCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchSuggestionCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
}

- (NSDictionary *)selectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: UIColor.linkBlue, NSFontAttributeName: UIFont.bold16};
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: UIColor.linkBlue, NSFontAttributeName: UIFont.regular16};
}

#pragma mark - Properties

- (void)setIsLastCell:(BOOL)isLastCell
{
  _isLastCell = isLastCell;
  self.separatorInset = UIEdgeInsetsMake(0, isLastCell ? 0 : kSearchCellSeparatorInset, 0, 0);
}

@end
