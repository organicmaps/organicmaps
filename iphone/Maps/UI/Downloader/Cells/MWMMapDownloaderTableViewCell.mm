#import "MWMMapDownloaderTableViewCell.h"
#import "MWMCommon.h"
#import "MWMCircularProgress.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "NSString+Categories.h"

#include "Framework.h"

@interface MWMMapDownloaderTableViewCell ()<MWMCircularProgressProtocol>

@property(nonatomic) MWMCircularProgress * progress;
@property(copy, nonatomic) NSString * searchQuery;

@property(weak, nonatomic) IBOutlet UIView * stateWrapper;
@property(weak, nonatomic) IBOutlet UILabel * title;
@property(weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@implementation MWMMapDownloaderTableViewCell
{
  storage::TCountryId m_countryId;
}

+ (CGFloat)estimatedHeight { return 52.0; }
- (void)awakeFromNib
{
  [super awakeFromNib];
  m_countryId = kInvalidCountryId;
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  m_countryId = kInvalidCountryId;
}

#pragma mark - Search matching

- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs
{
  NSMutableAttributedString * attrTitle = [[NSMutableAttributedString alloc] initWithString:str];
  [attrTitle addAttributes:unselectedAttrs range:{0, str.length}];
  if (!self.searchQuery)
    return [attrTitle copy];
  for (auto const & range : [str rangesOfString:self.searchQuery])
    [attrTitle addAttributes:selectedAttrs range:range];
  return [attrTitle copy];
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [self configProgress:nodeAttrs];

  NSDictionary * const selectedTitleAttrs = @{NSFontAttributeName : [UIFont bold17]};
  NSDictionary * const unselectedTitleAttrs = @{NSFontAttributeName : [UIFont regular17]};
  self.title.attributedText = [self matchedString:@(nodeAttrs.m_nodeLocalName.c_str())
                                    selectedAttrs:selectedTitleAttrs
                                  unselectedAttrs:unselectedTitleAttrs];

  TMwmSize size = nodeAttrs.m_mwmSize;
  bool const isModeDownloaded = self.mode == MWMMapDownloaderModeDownloaded;

  switch (nodeAttrs.m_status)
  {
  case storage::NodeStatus::Error:
  case storage::NodeStatus::Undefined:
  case storage::NodeStatus::NotDownloaded:
  case storage::NodeStatus::OnDiskOutOfDate:
    size = isModeDownloaded ? nodeAttrs.m_localMwmSize : nodeAttrs.m_mwmSize;
    break;
  case storage::NodeStatus::Downloading:
    size = isModeDownloaded ? nodeAttrs.m_downloadingMwmSize
                            : nodeAttrs.m_mwmSize - nodeAttrs.m_downloadingMwmSize;
    break;
  case storage::NodeStatus::InQueue:
  case storage::NodeStatus::Partly:
    size = isModeDownloaded ? nodeAttrs.m_localMwmSize
                            : nodeAttrs.m_mwmSize;
    break;
  case storage::NodeStatus::OnDisk: size = isModeDownloaded ? nodeAttrs.m_mwmSize : 0; break;
  }

  self.downloadSize.text = formattedSize(size);
  self.downloadSize.hidden = (size == 0);
}

- (void)configProgress:(storage::NodeAttrs const &)nodeAttrs
{
  MWMCircularProgress * progress = self.progress;
  MWMButtonColoring const coloring =
      self.mode == MWMMapDownloaderModeDownloaded ? MWMButtonColoringBlack : MWMButtonColoringBlue;
  switch (nodeAttrs.m_status)
  {
  case NodeStatus::NotDownloaded:
  case NodeStatus::Partly:
  {
    MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
                                                        MWMCircularProgressStateSelected};
    NSString * imageName = [self isKindOfClass:[MWMMapDownloaderLargeCountryTableViewCell class]]
                          ? @"ic_folder" : @"ic_download";
    [progress setImageName:imageName forStates:affectedStates];
    [progress setColoring:coloring forStates:affectedStates];
    progress.state = MWMCircularProgressStateNormal;
    break;
  }
  case NodeStatus::Downloading:
  {
    auto const & prg = nodeAttrs.m_downloadingProgress;
    // Here we left 5% for diffs applying.
    progress.progress = 0.95f * static_cast<CGFloat>(prg.first) / prg.second;
    break;
  }
  case NodeStatus::InQueue: progress.state = MWMCircularProgressStateSpinner; break;
  case NodeStatus::Undefined:
  case NodeStatus::Error: progress.state = MWMCircularProgressStateFailed; break;
  case NodeStatus::OnDisk: progress.state = MWMCircularProgressStateCompleted; break;
  case NodeStatus::OnDiskOutOfDate:
  {
    MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
                                                        MWMCircularProgressStateSelected};
    [progress setImageName:@"ic_update" forStates:affectedStates];
    [progress setColoring:MWMButtonColoringOther forStates:affectedStates];
    progress.state = MWMCircularProgressStateNormal;
    break;
  }
  }
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (countryId != m_countryId)
    return;
  storage::NodeAttrs nodeAttrs;
  GetFramework().GetStorage().GetNodeAttrs(m_countryId, nodeAttrs);
  [self config:nodeAttrs];
}

- (void)processCountry:(TCountryId const &)countryId
              progress:(MapFilesDownloader::TProgress const &)progress
{
  if (countryId != m_countryId)
    return;
  // Here we left 5% for diffs applying.
  self.progress.progress = 0.95f * static_cast<CGFloat>(progress.first) / progress.second;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  storage::NodeAttrs nodeAttrs;
  GetFramework().GetStorage().GetNodeAttrs(m_countryId, nodeAttrs);
  id<MWMMapDownloaderProtocol> delegate = self.delegate;
  switch (nodeAttrs.m_status)
  {
  case NodeStatus::NotDownloaded:
  case NodeStatus::Partly:
    if ([self isKindOfClass:[MWMMapDownloaderLargeCountryTableViewCell class]])
      [delegate openNodeSubtree:m_countryId];
    else
      [delegate downloadNode:m_countryId];
    break;
  case NodeStatus::Undefined:
  case NodeStatus::Error: [delegate retryDownloadNode:m_countryId]; break;
  case NodeStatus::OnDiskOutOfDate: [delegate updateNode:m_countryId]; break;
  case NodeStatus::Downloading:
  case NodeStatus::InQueue: [delegate cancelNode:m_countryId]; break;
  case NodeStatus::OnDisk: break;
  }
}

#pragma mark - Properties

- (void)setCountryId:(NSString *)countryId searchQuery:(NSString *)query
{
  if (m_countryId == countryId.UTF8String && [query isEqualToString:self.searchQuery])
    return;
  self.searchQuery = query;
  m_countryId = countryId.UTF8String;
  storage::NodeAttrs nodeAttrs;
  GetFramework().GetStorage().GetNodeAttrs(m_countryId, nodeAttrs);
  [self config:nodeAttrs];
}

- (MWMCircularProgress *)progress
{
  if (!_progress && !self.isHeightCell)
  {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.stateWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

@end
