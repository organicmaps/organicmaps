#import "MWMMapDownloaderCountryTableViewCell.h"

@implementation MWMMapDownloaderCountryTableViewCell

- (void)layoutSubviews
{
  CGFloat const titleLeadingOffset = 60.0;
  CGFloat const titleTrailingOffset = 80.0;
  CGFloat const preferredMaxLayoutWidth = CGRectGetWidth(self.bounds) - titleLeadingOffset - titleTrailingOffset;
  self.title.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 52.0;
}

@end
