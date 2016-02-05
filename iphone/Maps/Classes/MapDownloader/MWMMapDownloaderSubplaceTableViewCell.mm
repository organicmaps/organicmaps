#import "MWMMapDownloaderSubplaceTableViewCell.h"

@interface MWMMapDownloaderTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleLeadingOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleSizeOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * downloadSizeTrailingOffset;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

- (void)layoutSubviews
{
  CGFloat const preferredMaxLayoutWidth =
      CGRectGetWidth(self.bounds) - self.titleLeadingOffset.constant -
      self.titleSizeOffset.constant - CGRectGetWidth(self.downloadSize.bounds) -
      self.downloadSizeTrailingOffset.constant;
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
