#import "MWMSearchCell.h"
#import "SearchResult.h"

@interface MWMSearchCell ()

@property(weak, nonatomic) IBOutlet UILabel * titleLabel;

@end

@implementation MWMSearchCell

- (void)configureWith:(SearchResult * _Nonnull)result isPartialMatching:(BOOL)isPartialMatching
{
  NSString * title = result.titleText;

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
  NSMutableAttributedString * attributedTitle = [[NSMutableAttributedString alloc] initWithString:title];
  NSDictionary * titleAttributes = isPartialMatching ? unselectedTitleAttributes : selectedTitleAttributes;

  NSArray<NSValue *> * highlightRanges = result.highlightRanges;
  [attributedTitle addAttributes:titleAttributes range:NSMakeRange(0, title.length)];

  for (NSValue * rangeValue in highlightRanges)
  {
    NSRange range = [rangeValue rangeValue];
    if (NSMaxRange(range) <= result.titleText.length)
      [attributedTitle addAttributes:selectedTitleAttributes range:range];
    else
      NSLog(@"Incorrect range: %@ for string: %@", NSStringFromRange(range), result.titleText);
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
