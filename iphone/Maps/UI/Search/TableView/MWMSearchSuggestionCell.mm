#import "MWMSearchSuggestionCell.h"
#import "SwiftBridge.h"

@interface MWMSearchSuggestionCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchSuggestionCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor whitePrimary];
}

- (NSDictionary *)selectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: [UIColor linkBlue], NSFontAttributeName: UIFont.bold16.dynamic};
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: [UIColor linkBlue], NSFontAttributeName: UIFont.regular16.dynamic};
}

#pragma mark - Properties

- (void)setIsLastCell:(BOOL)isLastCell
{
  _isLastCell = isLastCell;
  self.separatorInset = UIEdgeInsetsMake(0, isLastCell ? 0 : kSearchCellSeparatorInset, 0, 0);
}

@end
