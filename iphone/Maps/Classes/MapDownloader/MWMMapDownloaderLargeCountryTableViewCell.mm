#import "MWMMapDownloaderLargeCountryTableViewCell.h"

@implementation MWMMapDownloaderLargeCountryTableViewCell

- (void)layoutSubviews
{
  CGFloat const titleLeadingOffset = 60.0;
  CGFloat const titleTrailingOffset = 112.0;
  CGFloat const preferredMaxLayoutWidth = CGRectGetWidth(self.bounds) - titleLeadingOffset - titleTrailingOffset;
  self.title.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  self.mapsCount.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
