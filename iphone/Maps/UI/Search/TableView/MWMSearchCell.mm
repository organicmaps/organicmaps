#import "MWMSearchCell.h"

#include <CoreApi/Framework.h>

@interface MWMSearchCell ()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;

@end

@implementation MWMSearchCell

- (void)config:(search::Result const &)result
    localizedTypeName:(NSString *)localizedTypeName
{
  NSString * title = result.GetString().empty() ? localizedTypeName : @(result.GetString().c_str());
  
  if (title.length == 0)
  {
    self.titleLabel.text = @"";
    return;
  }
  
  NSDictionary * selectedTitleAttributes = [self selectedTitleAttributes];
  NSDictionary * unselectedTitleAttributes = [self unselectedTitleAttributes];
  if (!selectedTitleAttributes || !unselectedTitleAttributes)
  {
    self.titleLabel.text = title;
    return;
  }
  NSMutableAttributedString * attributedTitle =
      [[NSMutableAttributedString alloc] initWithString:title];
  [attributedTitle addAttributes:unselectedTitleAttributes range:NSMakeRange(0, title.length)];
  size_t const rangesCount = result.GetHighlightRangesCount();
  for (size_t i = 0; i < rangesCount; ++i)
  {
    std::pair<uint16_t, uint16_t> const & range = result.GetHighlightRange(i);

    if (range.first + range.second <= title.length)
    {
      [attributedTitle addAttributes:selectedTitleAttributes range:NSMakeRange(range.first, range.second)];
    }
    else
    {
      LOG(LERROR, ("Incorrect range: ", range, " for string: ", result.GetString()));
    }
  }
  self.titleLabel.attributedText = attributedTitle;
  [self.titleLabel sizeToFit];
}

- (NSDictionary *)selectedTitleAttributes
{
  return nil;
}

- (NSDictionary *)unselectedTitleAttributes
{
  return nil;
}

@end
