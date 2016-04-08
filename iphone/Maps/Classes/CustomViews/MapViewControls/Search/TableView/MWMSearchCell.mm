#import "MWMSearchCell.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

#include "base/logging.hpp"

@interface MWMSearchCell ()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;

@end

@implementation MWMSearchCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)config:(search::Result &)result
{
  if (result.GetResultType() == search::Result::RESULT_FEATURE)
    GetFramework().LoadSearchResultMetadata(result);

  NSString * title = @(result.GetString().c_str());
  if (!title)
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
    pair<uint16_t, uint16_t> const & range = result.GetHighlightRange(i);

    if (range.first + range.second <= title.length)
    {
      [attributedTitle addAttributes:selectedTitleAttributes range:NSMakeRange(range.first, range.second)];
    }
    else
    {
      [Statistics logEvent:@"Incorrect_highlight_range" withParameters:@{@"range.first" : @(range.first),
                                                                        @"range.second" : @(range.second),
                                                                        @"string" : title}];
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
