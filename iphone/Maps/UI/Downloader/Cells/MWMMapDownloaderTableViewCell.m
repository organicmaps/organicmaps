#import "MWMMapDownloaderTableViewCell.h"
#import "MWMCircularProgress.h"
#import "NSString+Categories.h"

#import <CoreApi/MWMCommon.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <CoreApi/MWMMapNodeAttributes.h>

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property(copy, nonatomic) NSString *searchQuery;

@property(weak, nonatomic) IBOutlet UIView *stateWrapper;
@property(weak, nonatomic) IBOutlet UILabel *title;
@property(weak, nonatomic) IBOutlet UILabel *downloadSize;

@property(strong, nonatomic) MWMMapNodeAttributes *nodeAttrs;

@end

@implementation MWMMapDownloaderTableViewCell

- (void)awakeFromNib {
  [super awakeFromNib];
  UILongPressGestureRecognizer *lpGR = [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                                                     action:@selector(onLongPress:)];
  [self addGestureRecognizer:lpGR];
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.nodeAttrs = nil;
}

- (void)onLongPress:(UILongPressGestureRecognizer *)sender {
  if (sender.state != UIGestureRecognizerStateBegan) {
    return;
  }
  [self.delegate mapDownloaderCellDidLongPress:self];
}

#pragma mark - Search matching

- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs {
  NSMutableAttributedString *attrTitle = [[NSMutableAttributedString alloc] initWithString:str];
  [attrTitle addAttributes:unselectedAttrs range:NSMakeRange(0, str.length)];
  if (!self.searchQuery)
    return [attrTitle copy];
  for (NSValue *range in [str rangesOfString:self.searchQuery])
    [attrTitle addAttributes:selectedAttrs range:range.rangeValue];
  return [attrTitle copy];
}

#pragma mark - Config

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(NSString *)searchQuery {
  self.searchQuery = searchQuery;
  self.nodeAttrs = nodeAttrs;
  [self configProgress:nodeAttrs];

  self.title.attributedText = [self matchedString:nodeAttrs.nodeName
                                    selectedAttrs:@{NSFontAttributeName: [UIFont bold17]}
                                  unselectedAttrs:@{NSFontAttributeName: [UIFont regular17]}];

  uint64_t size = 0;
  BOOL isModeDownloaded = self.mode == MWMMapDownloaderModeDownloaded;

  switch (nodeAttrs.nodeStatus) {
    case MWMMapNodeStatusUndefined:
    case MWMMapNodeStatusError:
    case MWMMapNodeStatusOnDiskOutOfDate:
    case MWMMapNodeStatusNotDownloaded:
    case MWMMapNodeStatusApplying:
    case MWMMapNodeStatusInQueue:
    case MWMMapNodeStatusPartly:
      size = isModeDownloaded ? nodeAttrs.downloadedSize : nodeAttrs.totalSize;
      break;
    case MWMMapNodeStatusDownloading:
      size = isModeDownloaded ? nodeAttrs.totalUpdateSizeBytes : nodeAttrs.totalSize - nodeAttrs.downloadingSize;
      break;
    case MWMMapNodeStatusOnDisk:
      size = isModeDownloaded ? nodeAttrs.totalSize : 0;
      break;
  }

  self.downloadSize.text = formattedSize(size);
  self.downloadSize.hidden = (size == 0);
}

- (void)configProgress:(MWMMapNodeAttributes *)nodeAttrs {
  MWMCircularProgress *progress = self.progress;
  BOOL isModeDownloaded = self.mode == MWMMapDownloaderModeDownloaded;
  MWMButtonColoring coloring = isModeDownloaded ? MWMButtonColoringBlack : MWMButtonColoringBlue;
  switch (nodeAttrs.nodeStatus) {
    case MWMMapNodeStatusNotDownloaded:
    case MWMMapNodeStatusPartly: {
      MWMCircularProgressStateVec affectedStates = @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];
      [progress setImageName:@"ic_download" forStates:affectedStates];
      [progress setColoring:coloring forStates:affectedStates];
      progress.state = MWMCircularProgressStateNormal;
      break;
    }
    case MWMMapNodeStatusDownloading:
      progress.progress = kMaxProgress * nodeAttrs.downloadedSize / (isModeDownloaded ? nodeAttrs.totalUpdateSizeBytes : nodeAttrs.totalSize - nodeAttrs.downloadingSize);
      break;
    case MWMMapNodeStatusApplying:
    case MWMMapNodeStatusInQueue:
      progress.state = MWMCircularProgressStateSpinner;
      break;
    case MWMMapNodeStatusUndefined:
    case MWMMapNodeStatusError:
      progress.state = MWMCircularProgressStateFailed;
      break;
    case MWMMapNodeStatusOnDisk:
      progress.state = MWMCircularProgressStateCompleted;
      break;
    case MWMMapNodeStatusOnDiskOutOfDate: {
      MWMCircularProgressStateVec affectedStates = @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];
      [progress setImageName:@"ic_update" forStates:affectedStates];
      [progress setColoring:MWMButtonColoringOther forStates:affectedStates];
      progress.state = MWMCircularProgressStateNormal;
      break;
    }
  }
}

- (void)setDownloadProgress:(CGFloat)progress {
  self.progress.progress = kMaxProgress * progress;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress {
  [self.delegate mapDownloaderCellDidPressProgress:self];
}

#pragma mark - Properties

- (MWMCircularProgress *)progress {
  if (!_progress) {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.stateWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

@end
