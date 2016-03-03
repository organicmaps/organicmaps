#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMCircularProgress.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapDownloadDialog.h"
#import "MWMStorage.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

#include "storage/index.hpp"

extern char const * kAutoDownloadEnabledKey;

using namespace storage;

@interface MWMMapDownloadDialog ()<MWMFrameworkStorageObserver,
                                   MWMCircularProgressProtocol>
@property (weak, nonatomic) IBOutlet UILabel * parentNode;
@property (weak, nonatomic) IBOutlet UILabel * node;
@property (weak, nonatomic) IBOutlet UILabel * nodeSize;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * nodeTopOffset;
@property (weak, nonatomic) IBOutlet UIButton * downloadButton;
@property (weak, nonatomic) IBOutlet UIView * progressWrapper;

@property (weak, nonatomic) MWMViewController * controller;

@property (nonatomic) MWMCircularProgress * progressView;

@end

@implementation MWMMapDownloadDialog
{
  TCountryId m_countryId;
}

+ (instancetype)dialogForController:(MWMViewController *)controller
{
  MWMMapDownloadDialog * dialog = [[NSBundle mainBundle] loadNibNamed:[self className] owner:nil options:nil].firstObject;
  dialog.autoresizingMask = UIViewAutoresizingFlexibleHeight;
  dialog.controller = controller;
  return dialog;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.size = [self systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  UIView * superview = self.superview;
  self.center = {superview.midX, superview.midY};
  [super layoutSubviews];
}

- (void)configDialog
{
  auto & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(m_countryId, nodeAttrs);
  BOOL const isMultiParent = nodeAttrs.m_parentInfo.size() > 1;
  BOOL const noParrent = (nodeAttrs.m_parentInfo[0].m_id == s.GetRootId());
  BOOL const hideParent = (noParrent || isMultiParent);
  self.parentNode.hidden = hideParent;
  self.nodeTopOffset.priority = hideParent ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
  if (!hideParent)
    self.parentNode.text = @(nodeAttrs.m_parentInfo[0].m_localName.c_str());
  self.node.text = @(nodeAttrs.m_nodeLocalName.c_str());
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = formattedSize(nodeAttrs.m_mwmSize);

  switch (nodeAttrs.m_status)
  {
    case NodeStatus::NotDownloaded:
    {
      bool autoDownloadEnabled = true;
      (void)Settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled);
      if (autoDownloadEnabled)
      {
        [self showInQueue];
        s.DownloadNode(m_countryId);
      }
      else
      {
        [self showDownloadRequest];
      }
      [self addToSuperview];
      break;
    }
    case NodeStatus::Downloading:
      if (nodeAttrs.m_downloadingProgress.second != 0)
        [self showDownloading:static_cast<CGFloat>(nodeAttrs.m_downloadingProgress.first) / nodeAttrs.m_downloadingProgress.second];
      [self addToSuperview];
      break;
    case NodeStatus::InQueue:
      [self showInQueue];
      [self addToSuperview];
      break;
    case NodeStatus::Undefined:
    case NodeStatus::Error:
      [self showError:nodeAttrs.m_error];
      break;
    case NodeStatus::OnDisk:
    case NodeStatus::OnDiskOutOfDate:
      [self removeFromSuperview];
      break;
  }
}

- (void)addToSuperview
{
  if (self.superview)
    return;
  [self.controller.view insertSubview:self atIndex:0];
  [MWMFrameworkListener addObserver:self];
}

- (void)removeFromSuperview
{
  self.progressView.state = MWMCircularProgressStateNormal;
  [MWMFrameworkListener removeObserver:self];
  [super removeFromSuperview];
}

- (void)showError:(NodeErrorCode)errorCode
{
  if (errorCode == NodeErrorCode::NoError)
    return;
  self.nodeSize.textColor = [UIColor red];
  self.nodeSize.text = L(@"country_status_download_failed");
  self.progressView.state = MWMCircularProgressStateFailed;
  MWMAlertViewController * avc = self.controller.alertController;
  switch (errorCode)
  {
    case NodeErrorCode::NoError:
      break;
    case NodeErrorCode::UnknownError:
      [avc presentInternalErrorAlert];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [avc presentDownloaderNotEnoughSpaceAlert];
      break;
    case NodeErrorCode::NoInetConnection:
      [avc presentDownloaderNoConnectionAlertWithOkBlock:^
      {
        [MWMStorage retryDownloadNode:self->m_countryId];
      }];
      break;
  }
}

- (void)showDownloadRequest
{
  self.downloadButton.hidden = NO;
  self.progressWrapper.hidden = YES;
}

- (void)showDownloading:(CGFloat)progress
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = [@(static_cast<NSUInteger>(progress * 100)).stringValue stringByAppendingString:@"%"];
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progressView.progress = progress;
}

- (void)showInQueue
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = L(@"downloader_queued");
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progressView.state = MWMCircularProgressStateSpinner;
}

- (void)processViewportCountryEvent:(TCountryId const &)countryId
{
  if (m_countryId == countryId)
    return;
  m_countryId = countryId;
  if (countryId == kInvalidCountryId)
    [self removeFromSuperview];
  else
    [self configDialog];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (m_countryId != countryId)
    return;

  if (self.superview && [self.controller.navigationController.topViewController isEqual:self.controller])
    [self configDialog];
  else
    [self removeFromSuperview];
}

- (void)processCountry:(TCountryId const &)countryId progress:(TLocalAndRemoteSize const &)progress
{
  if (self.superview && m_countryId == countryId)
    [self showDownloading:static_cast<CGFloat>(progress.first) / progress.second];
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  if (progress.state == MWMCircularProgressStateFailed)
  {
    [self showInQueue];
    [MWMStorage retryDownloadNode:m_countryId];
  }
  else
  {
    [self showDownloadRequest];
    [MWMStorage cancelDownloadNode:m_countryId];
  }
}

#pragma mark - Actions

- (IBAction)downloadAction
{
  [MWMStorage downloadNode:m_countryId alertController:self.controller.alertController onSuccess:nil];
}

#pragma mark - Properties

- (MWMCircularProgress *)progressView
{
  if (!_progressView)
  {
    _progressView = [[MWMCircularProgress alloc] initWithParentView:self.progressWrapper];
    _progressView.delegate = self;
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateNormal];
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateSelected];
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
    [_progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateSpinner];
    [_progressView setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
    [_progressView setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
  }
  return _progressView;
}

@end
