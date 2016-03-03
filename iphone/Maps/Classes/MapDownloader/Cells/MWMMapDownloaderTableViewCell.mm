#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMMapDownloaderTableViewCell.h"

#include "Framework.h"

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property (nonatomic) MWMCircularProgress * progressView;
@property (weak, nonatomic) IBOutlet UIView * stateWrapper;
@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@implementation MWMMapDownloaderTableViewCell
{
  storage::TCountryId m_countryId;
}

+ (CGFloat)estimatedHeight
{
  return 52.0;
}

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

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.title.preferredMaxLayoutWidth = floor(self.title.width);
    self.downloadSize.preferredMaxLayoutWidth = floor(self.downloadSize.width);
    [super layoutSubviews];
  }
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [self configProgressView:nodeAttrs];
  self.title.text = @(nodeAttrs.m_nodeLocalName.c_str());
  self.downloadSize.text = formattedSize(nodeAttrs.m_mwmSize);
}

- (void)configProgressView:(const storage::NodeAttrs &)nodeAttrs
{
  switch (nodeAttrs.m_status)
  {
    case NodeStatus::NotDownloaded:
      self.progressView.state = MWMCircularProgressStateNormal;
      break;
    case NodeStatus::Downloading:
    {
      auto const & progress = nodeAttrs.m_downloadingProgress;
      self.progressView.progress = static_cast<CGFloat>(progress.first) / progress.second;
      break;
    }
    case NodeStatus::InQueue:
      self.progressView.state = MWMCircularProgressStateSpinner;
      break;
    case NodeStatus::Undefined:
    case NodeStatus::Error:
      self.progressView.state = MWMCircularProgressStateFailed;
      break;
    case NodeStatus::OnDisk:
      self.progressView.state = MWMCircularProgressStateCompleted;
      break;
    case NodeStatus::OnDiskOutOfDate:
      self.progressView.state = MWMCircularProgressStateSelected;
      break;
  }
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (countryId != m_countryId)
    return;
  storage::NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(m_countryId, nodeAttrs);
  [self config:nodeAttrs];
}

- (void)processCountry:(TCountryId const &)countryId progress:(TLocalAndRemoteSize const &)progress
{
  if (countryId != m_countryId)
    return;
  self.progressView.progress = static_cast<CGFloat>(progress.first) / progress.second;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  storage::NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(m_countryId, nodeAttrs);
  switch (nodeAttrs.m_status)
  {
    case NodeStatus::NotDownloaded:
      [self.delegate downloadNode:m_countryId];
      break;
    case NodeStatus::Undefined:
    case NodeStatus::Error:
      [self.delegate retryDownloadNode:m_countryId];
      break;
    case NodeStatus::OnDiskOutOfDate:
      [self.delegate updateNode:m_countryId];
      break;
    case NodeStatus::Downloading:
    case NodeStatus::InQueue:
      [self.delegate cancelNode:m_countryId];
      break;
    case NodeStatus::OnDisk:
      break;
  }
}

#pragma mark - Properties

- (void)setCountryId:(storage::TCountryId const &)countryId
{
  if (m_countryId == countryId)
    return;
  m_countryId = countryId;
  storage::NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(m_countryId, nodeAttrs);
  [self config:nodeAttrs];
}

- (MWMCircularProgress *)progressView
{
  if (!_progressView && !self.isHeightCell)
  {
    _progressView = [[MWMCircularProgress alloc] initWithParentView:self.stateWrapper];
    _progressView.delegate = self;
    [_progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateNormal];
    [_progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateSelected];
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateSpinner];
    [_progressView setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
    [_progressView setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
  }
  return _progressView;
}

@end
