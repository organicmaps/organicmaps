#import "MWMMapDownloaderLargeCountryTableViewCell.h"

@interface MWMMapDownloaderTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@interface MWMMapDownloaderLargeCountryTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * mapsCount;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleLeadingOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleSizeOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * downloadSizeTrailingOffset;

@end

@implementation MWMMapDownloaderLargeCountryTableViewCell

- (void)layoutSubviews
{
  CGFloat const preferredMaxLayoutWidth =
      CGRectGetWidth(self.bounds) - self.titleLeadingOffset.constant -
      self.titleSizeOffset.constant - CGRectGetWidth(self.downloadSize.bounds) -
      self.downloadSizeTrailingOffset.constant;
  self.title.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  self.mapsCount.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [super layoutSubviews];
}

- (void)setMapCountText:(NSString *)text
{
  self.mapsCount.text = text;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
