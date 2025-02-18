#import "MWMSearchCell.h"
#import "SearchResult.h"

@interface MWMSearchCell ()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;

@end

@implementation MWMSearchCell

- (void)configureWith:(SearchResult * _Nonnull)result {
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
  NSMutableAttributedString * attributedTitle =
      [[NSMutableAttributedString alloc] initWithString:title];
  [attributedTitle addAttributes:unselectedTitleAttributes range:NSMakeRange(0, title.length)];

  NSArray<NSValue *> *highlightRanges = result.highlightRanges;

  for (NSValue *rangeValue in highlightRanges) {
    NSRange range = [rangeValue rangeValue];
    if (NSMaxRange(range) <= result.titleText.length) {
      [attributedTitle addAttributes:selectedTitleAttributes range:range];
    } else {
      NSLog(@"Incorrect range: %@ for string: %@", NSStringFromRange(range), result.titleText);
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
