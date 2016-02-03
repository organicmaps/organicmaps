#import "MWMMapDownloaderSubplaceTableViewCell.h"

@implementation MWMMapDownloaderSubplaceTableViewCell

- (void)layoutSubviews
{
  CGFloat const titleLeadingOffset = 60.0;
  CGFloat const titleTrailingOffset = 80.0;
  CGFloat const preferredMaxLayoutWidth = CGRectGetWidth(self.bounds) - titleLeadingOffset - titleTrailingOffset;
  self.title.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  self.area.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  self.subPlace.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 82.0;
}

@end
