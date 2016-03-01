#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMFrameworkListener.h"
#import "MWMMapDownloaderTableViewCell.h"

#include "Framework.h"

@interface MWMMapDownloaderTableViewCell () <MWMFrameworkStorageObserver, MWMCircularProgressProtocol>

@property (nonatomic) MWMCircularProgress * progressView;
@property (weak, nonatomic) IBOutlet UIView * stateWrapper;
@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@property (nonatomic) BOOL isObserver;

@end

@implementation MWMMapDownloaderTableViewCell
{
  storage::TCountryId m_countryId;
}

#pragma mark - Properties

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self reset];
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  [self reset];
}

- (void)reset
{
  self.progressView = [[MWMCircularProgress alloc] initWithParentView:self.stateWrapper];
  self.progressView.delegate = self;
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateNormal];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateSelected];
  [self.progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
  [self.progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateSpinner];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
  [self.progressView setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
  m_countryId = kInvalidCountryId;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.title.preferredMaxLayoutWidth = self.title.width;
  self.downloadSize.preferredMaxLayoutWidth = self.downloadSize.width;
  [super layoutSubviews];
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
      self.progressView.progress = static_cast<CGFloat>(nodeAttrs.m_downloadingProgress.first) / 100.0;
      break;
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

#pragma mark - Framework observer

- (void)registerObserver
{
  if (self.isObserver)
    return;
  [MWMFrameworkListener addObserver:self];
  self.isObserver = YES;
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

- (CGFloat)estimatedHeight
{
  return 52.0;
}

- (void)setCountryId:(storage::TCountryId const &)countryId
{
  if (m_countryId == countryId)
    return;
  m_countryId = countryId;
  storage::NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(m_countryId, nodeAttrs);
  [self config:nodeAttrs];
}

@end
