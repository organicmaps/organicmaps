#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMCircularProgress.h"

#import <CoreApi/MWMMapNodeAttributes.h>

@interface MWMMapDownloaderLargeCountryTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * mapsCount;

@end

@implementation MWMMapDownloaderLargeCountryTableViewCell

#pragma mark - Config

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(NSString *)searchQuery {
  [super config:nodeAttrs searchQuery:searchQuery];
  BOOL haveLocalMaps = (nodeAttrs.downloadedMwmCount != 0);
  NSString *ofMaps = haveLocalMaps ? [NSString stringWithFormat:L(@"downloader_of"), nodeAttrs.downloadedMwmCount, nodeAttrs.totalMwmCount] : @(nodeAttrs.totalMwmCount).stringValue;
  self.mapsCount.text = [NSString stringWithFormat:@"%@: %@", L(@"downloader_status_maps"), ofMaps];
}

- (void)configProgress:(MWMMapNodeAttributes *)nodeAttrs {
  [super configProgress:nodeAttrs];
  if (nodeAttrs.nodeStatus == MWMMapNodeStatusPartly || nodeAttrs.nodeStatus == MWMMapNodeStatusNotDownloaded) {
    MWMCircularProgressStateVec affectedStates = @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];
    [self.progress setImageName:@"ic_folder" forStates:affectedStates];
  }
}

@end
