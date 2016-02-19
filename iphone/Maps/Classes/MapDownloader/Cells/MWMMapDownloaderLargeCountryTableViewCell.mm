#import "MWMMapDownloaderLargeCountryTableViewCell.h"

@interface MWMMapDownloaderLargeCountryTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * mapsCount;

@end

@implementation MWMMapDownloaderLargeCountryTableViewCell

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.mapsCount.preferredMaxLayoutWidth = self.mapsCount.width;
  [super layoutSubviews];
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  self.mapsCount.text = @(nodeAttrs.m_mwmCounter).stringValue;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
