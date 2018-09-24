#import "MWMCommon.h"
#import "MWMSearchCell.h"
#import "Statistics.h"

#include "Framework.h"

#include "indexer/classificator.hpp"

#include "platform/localization.hpp"

#include "base/logging.hpp"

#include <string>

@interface MWMSearchCell ()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;

@end

@implementation MWMSearchCell

- (void)config:(search::Result const &)result
{
  NSString * title = @(result.GetString().c_str());
  if (title.length == 0)
  {
    title = [self.class getLocalizedTypeName:result];
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

  self.backgroundColor = [UIColor white];
}

+ (NSString *)getLocalizedTypeName:(search::Result const &)result
{
  if (result.GetResultType() != search::Result::Type::Feature)
    return @"";

  auto const readableType = classif().GetReadableObjectName(result.GetFeatureType());

  return @(platform::GetLocalizedTypeName(readableType).c_str());
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
