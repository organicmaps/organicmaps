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
  BOOL const haveLocalMaps = (nodeAttrs.m_localMwmCounter != 0);
  self.mapsCount.text =
      haveLocalMaps
          ? [NSString stringWithFormat:@"%@: %@ %@ %@", L(@"downloader_maps"),
                                       @(nodeAttrs.m_localMwmCounter), L(@"_of"),
                                       @(nodeAttrs.m_mwmCounter)]
          : [NSString stringWithFormat:@"%@: %@", L(@"downloader_maps"), @(nodeAttrs.m_mwmCounter)];
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
