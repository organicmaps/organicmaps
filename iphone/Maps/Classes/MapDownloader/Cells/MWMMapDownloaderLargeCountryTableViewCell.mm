#import "Common.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"

@interface MWMMapDownloaderLargeCountryTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * mapsCount;

@end

@implementation MWMMapDownloaderLargeCountryTableViewCell

+ (CGFloat)estimatedHeight
{
  return 62.0;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.mapsCount.preferredMaxLayoutWidth = self.mapsCount.width;
    [super layoutSubviews];
  }
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  BOOL const haveLocalMaps = (nodeAttrs.m_localMwmCounter != 0);
  self.mapsCount.text =
      haveLocalMaps
          ? [NSString stringWithFormat:@"%@: %@ %@ %@", L(@"downloader_status_maps"),
                                       @(nodeAttrs.m_localMwmCounter), L(@"_of"),
                                       @(nodeAttrs.m_mwmCounter)]
          : [NSString stringWithFormat:@"%@: %@", L(@"downloader_status_maps"), @(nodeAttrs.m_mwmCounter)];
}

@end
