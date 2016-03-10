#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMCircularProgress.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapDownloadDialog.h"
#import "MWMStorage.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

extern char const * kAutoDownloadEnabledKey;

namespace
{
NSTimeInterval constexpr kDisableAutoDownloadInterval = 30;
NSInteger constexpr kDisableAutoDownloadCount = 3;
CGSize constexpr kInitialDialogSize = {200, 200};

BOOL isAutoDownload()
{
  bool autoDownloadEnabled = true;
  (void)Settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled);
  return autoDownloadEnabled && GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI;
}
} // namespace

using namespace storage;

@interface MWMMapDownloadDialog ()<MWMFrameworkStorageObserver,
                                   MWMCircularProgressProtocol>
@property (weak, nonatomic) IBOutlet UILabel * parentNode;
@property (weak, nonatomic) IBOutlet UILabel * node;
@property (weak, nonatomic) IBOutlet UILabel * nodeSize;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * nodeTopOffset;
@property (weak, nonatomic) IBOutlet UIButton * downloadButton;
@property (weak, nonatomic) IBOutlet UIView * progressWrapper;
@property (weak, nonatomic) IBOutlet UILabel * autoDownload;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * autoDownloadBottomOffset;
@property (weak, nonatomic) IBOutlet UIView * autoDownloadWrapper;
@property (weak, nonatomic) IBOutlet MWMButton * autoDownloadButton;

@property (weak, nonatomic) MWMViewController * controller;

@property (nonatomic) MWMCircularProgress * progress;

@property (nonatomic) NSMutableArray<NSDate *> * skipDownloadTimes;

@property (nonatomic) BOOL isCancelled;

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
  dialog.size = kInitialDialogSize;
  return dialog;
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.center = {superview.midX, superview.midY};
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.size = [self systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
    self.center = {superview.midX, superview.midY};
    [self layoutIfNeeded];
  }];
  [super layoutSubviews];
  if (isIOS7)
  {
    self.parentNode.preferredMaxLayoutWidth = floor(self.parentNode.width);
    self.node.preferredMaxLayoutWidth = floor(self.node.width);
    self.nodeSize.preferredMaxLayoutWidth = floor(self.nodeSize.width);
    self.autoDownload.preferredMaxLayoutWidth = floor(self.autoDownload.width);
    [super layoutSubviews];
  }
}

- (void)configDialog
{
  auto & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(m_countryId, nodeAttrs);

  BOOL const isMapVisible = [self.controller.navigationController.topViewController isEqual:self.controller];
  if (isMapVisible && !GetFramework().IsRoutingActive())
  {
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
      case NodeStatus::Partly:
      {
        if (!self.isCancelled && isAutoDownload())
        {
          [Statistics logEvent:kStatDownloaderMapAction
                withParameters:@{
                  kStatAction : kStatDownload,
                  kStatIsAuto : kStatYes,
                  kStatFrom : kStatMap,
                  kStatScenario : kStatDownload
                }];
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
  else
  {
    [self removeFromSuperview];
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
  self.progress.state = MWMCircularProgressStateNormal;
  [MWMFrameworkListener removeObserver:self];
  [super removeFromSuperview];
}

- (void)showError:(NodeErrorCode)errorCode
{
  if (errorCode == NodeErrorCode::NoError)
    return;
  self.nodeSize.textColor = [UIColor red];
  self.nodeSize.text = L(@"country_status_download_failed");
  self.progress.state = MWMCircularProgressStateFailed;
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
        [Statistics logEvent:kStatDownloaderMapAction
              withParameters:@{
                kStatAction : kStatRetry,
                kStatIsAuto : kStatNo,
                kStatFrom : kStatMap,
                kStatScenario : kStatDownload
              }];
        [MWMStorage retryDownloadNode:self->m_countryId];
      }];
      break;
  }
}

- (void)showAutoDownloadRequest:(BOOL)show
{
  if (show)
  {
    self.autoDownloadWrapper.hidden = NO;
    self.autoDownloadBottomOffset.priority = UILayoutPriorityDefaultHigh;
    bool autoDownloadEnabled = true;
    (void)Settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled);
    self.autoDownloadButton.selected = autoDownloadEnabled;
  }
  else
  {
    self.autoDownloadWrapper.hidden = YES;
    self.autoDownloadBottomOffset.priority = UILayoutPriorityDefaultLow;
  }
}

- (void)showDownloadRequest
{
  [self showAutoDownloadRequest:self.isCancelled && isAutoDownload()];
  self.isCancelled = NO;
  self.downloadButton.hidden = NO;
  self.progressWrapper.hidden = YES;
}

- (void)showDownloading:(CGFloat)progress
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = [@(static_cast<NSUInteger>(progress * 100)).stringValue stringByAppendingString:@"%"];
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.progress = progress;
  [self showAutoDownloadRequest:NO];
}

- (void)showInQueue
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = L(@"downloader_queued");
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.state = MWMCircularProgressStateSpinner;
  [self showAutoDownloadRequest:NO];
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

#pragma mark - Autodownload

- (void)cancelDownload
{
  self.isCancelled = YES;
  bool autoDownloadEnabled = true;
  (void)Settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled);
  if (!autoDownloadEnabled)
    return;

  NSDate * currentTime = [NSDate date];
  [self.skipDownloadTimes filterUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDate * evaluatedObject, NSDictionary<NSString *, id> * bindings)
  {
    return [currentTime timeIntervalSinceDate:evaluatedObject] < kDisableAutoDownloadInterval;
  }]];
  [self.skipDownloadTimes addObject:currentTime];
  if (self.skipDownloadTimes.count >= kDisableAutoDownloadCount)
  {
    [self.skipDownloadTimes removeAllObjects];
    MWMAlertViewController * ac = self.controller.alertController;
    [ac presentDisableAutoDownloadAlertWithOkBlock:^
    {
      Settings::Set(kAutoDownloadEnabledKey, false);
    }];
  }
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (m_countryId != countryId)
    return;
  if (self.superview)
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
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction : kStatRetry,
            kStatIsAuto : kStatNo,
            kStatFrom : kStatMap,
            kStatScenario : kStatDownload
          }];
    [self showInQueue];
    [MWMStorage retryDownloadNode:m_countryId];
  }
  else
  {
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatMap}];
    [self cancelDownload];
    [MWMStorage cancelDownloadNode:m_countryId];
  }
}

#pragma mark - Actions

- (IBAction)autoDownloadToggle
{
  self.autoDownloadButton.selected = !self.autoDownloadButton.selected;
  Settings::Set(kAutoDownloadEnabledKey, static_cast<bool>(self.autoDownloadButton.selected));
}

- (IBAction)downloadAction
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatDownload,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatMap,
          kStatScenario : kStatDownload
        }];
  [self showInQueue];
  [MWMStorage downloadNode:m_countryId alertController:self.controller.alertController onSuccess:nil];
}

#pragma mark - Properties

- (MWMCircularProgress *)progress
{
  if (!_progress)
  {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.progressWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

- (NSMutableArray<NSDate *> *)skipDownloadTimes
{
  if (!_skipDownloadTimes)
    _skipDownloadTimes = [@[] mutableCopy];
  return _skipDownloadTimes;
}

@end
