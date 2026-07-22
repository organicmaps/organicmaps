#import "MWMMapDownloaderTableViewCell.h"
#import "MWMCircularProgress.h"
#import "NSString+Ranges.h"
#import "SwiftBridge.h"

#import <CoreApi/MWMCommon.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <CoreApi/MWMMapNodeAttributes.h>

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property(copy, nonatomic) NSString * searchQuery;

@property(weak, nonatomic) IBOutlet UIView * stateWrapper;
@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet UILabel * downloadSize;

@property(strong, nonatomic) MWMMapNodeAttributes * nodeAttrs;

@end

@implementation MWMMapDownloaderTableViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  UILongPressGestureRecognizer * lpGR = [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                                                      action:@selector(onLongPress:)];
  [self addGestureRecognizer:lpGR];
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  self.nodeAttrs = nil;
  _isTerrainCell = NO;
}

- (void)onLongPress:(UILongPressGestureRecognizer *)sender
{
  if (sender.state != UIGestureRecognizerStateBegan)
    return;
  [self.delegate mapDownloaderCellDidLongPress:self];
}

#pragma mark - Search matching

- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs
{
  NSMutableAttributedString * attrTitle = [[NSMutableAttributedString alloc] initWithString:str];
  [attrTitle addAttributes:unselectedAttrs range:NSMakeRange(0, str.length)];
  if (!self.searchQuery)
    return [attrTitle copy];
  for (NSValue * range in [str rangesOfString:self.searchQuery])
    [attrTitle addAttributes:selectedAttrs range:range.rangeValue];
  return [attrTitle copy];
}

#pragma mark - Config

- (void)configTerrain:(MWMMapNodeAttributes *)nodeAttrs
{
  // TODO(terrain): move the titles into data/strings/strings.txt before shipping.
  self.nodeAttrs = nodeAttrs;
  _isTerrainCell = YES;

  NSString * subtitle;
  MWMCircularProgress * progress = self.progress;
  switch (nodeAttrs.terrainStatus)
  {
  case MWMTerrainStatusDownloading:
  {
    float const percent = nodeAttrs.terrainTotalSize > 0
                            ? (float)nodeAttrs.terrainDownloadedSize / nodeAttrs.terrainTotalSize : 0.f;
    subtitle = [NSString stringWithFormat:@"Terrain — %d%%", (int)(percent * 100)];
    progress.state = MWMCircularProgressStateProgress;
    progress.progress = percent;
    break;
  }
  case MWMTerrainStatusOnDisk:
    subtitle = @"Terrain — downloaded";
    progress.state = MWMCircularProgressStateCompleted;
    break;
  case MWMTerrainStatusPartly:
    subtitle = @"Terrain — partly downloaded";
    progress.state = MWMCircularProgressStateNormal;
    break;
  case MWMTerrainStatusFailed:
    subtitle = @"Terrain — failed, tap to retry";
    progress.state = MWMCircularProgressStateFailed;
    break;
  default:
    subtitle = @"Terrain";
    progress.state = MWMCircularProgressStateNormal;
    break;
  }
  self.title.text = subtitle;
  self.title.font = UIFont.regular14.dynamic;
  self.downloadSize.text = formattedSize(nodeAttrs.terrainTotalSize);
  self.downloadSize.hidden = nodeAttrs.terrainTotalSize == 0;
}

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(NSString *)searchQuery
{
  self.searchQuery = searchQuery;
  self.nodeAttrs = nodeAttrs;
  _isTerrainCell = NO;
  [self configProgress:nodeAttrs];

  self.title.attributedText = [self matchedString:nodeAttrs.nodeName
                                    selectedAttrs:@{NSFontAttributeName: UIFont.bold17.dynamic}
                                  unselectedAttrs:@{NSFontAttributeName: UIFont.regular17.dynamic}];

  uint64_t size = 0;
  BOOL isModeDownloaded = self.mode == MWMMapDownloaderModeDownloaded;

  switch (nodeAttrs.nodeStatus)
  {
  case MWMMapNodeStatusUndefined:
  case MWMMapNodeStatusError:
  case MWMMapNodeStatusOnDiskOutOfDate:
  case MWMMapNodeStatusNotDownloaded:
  case MWMMapNodeStatusApplying:
  case MWMMapNodeStatusInQueue:
  case MWMMapNodeStatusPartly: size = isModeDownloaded ? nodeAttrs.downloadedSize : nodeAttrs.totalSize; break;
  case MWMMapNodeStatusDownloading:
    size = isModeDownloaded ? nodeAttrs.totalUpdateSizeBytes : nodeAttrs.totalSize - nodeAttrs.downloadingSize;
    break;
  case MWMMapNodeStatusOnDisk: size = isModeDownloaded ? nodeAttrs.totalSize : 0; break;
  }

  self.downloadSize.text = formattedSize(size);
  self.downloadSize.hidden = (size == 0);
}

- (void)configProgress:(MWMMapNodeAttributes *)nodeAttrs
{
  MWMCircularProgress * progress = self.progress;
  BOOL isModeDownloaded = self.mode == MWMMapDownloaderModeDownloaded;
  MWMButtonColoring coloring = isModeDownloaded ? MWMButtonColoringBlack : MWMButtonColoringBlue;
  switch (nodeAttrs.nodeStatus)
  {
  case MWMMapNodeStatusNotDownloaded:
  case MWMMapNodeStatusPartly:
  {
    MWMCircularProgressStateVec affectedStates =
        @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];
    [progress setImageName:@"ic_download" forStates:affectedStates];
    [progress setColoring:coloring forStates:affectedStates];
    progress.state = MWMCircularProgressStateNormal;
    break;
  }
  case MWMMapNodeStatusDownloading:
    CGFloat const downloadProgress = nodeAttrs.downloadingProgress;
    if (downloadProgress > 0)
      [self setDownloadProgress:downloadProgress];
    break;
  case MWMMapNodeStatusApplying:
  case MWMMapNodeStatusInQueue: progress.state = MWMCircularProgressStateSpinner; break;
  case MWMMapNodeStatusUndefined:
  case MWMMapNodeStatusError: progress.state = MWMCircularProgressStateFailed; break;
  case MWMMapNodeStatusOnDisk: progress.state = MWMCircularProgressStateCompleted; break;
  case MWMMapNodeStatusOnDiskOutOfDate:
  {
    MWMCircularProgressStateVec affectedStates =
        @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];
    [progress setImageName:@"ic_update" forStates:affectedStates];
    [progress setColoring:MWMButtonColoringOther forStates:affectedStates];
    progress.state = MWMCircularProgressStateNormal;
    break;
  }
  }
}

- (void)setDownloadProgress:(CGFloat)progress
{
  self.progress.progress = kMaxProgress * progress;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  [self.delegate mapDownloaderCellDidPressProgress:self];
}

#pragma mark - Properties

- (MWMCircularProgress *)progress
{
  if (!_progress)
  {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.stateWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

@end
