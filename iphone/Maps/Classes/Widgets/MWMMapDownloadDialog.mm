#import "MWMMapDownloadDialog.h"
#import "CLLocation+Mercator.h"
#import "MWMAlertViewController.h"
#import "MWMBottomMenuViewController.h"
#import "MWMCircularProgress.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationManager.h"
#import "MWMRouter.h"
#import "MWMSettings.h"
#import "MWMStorage.h"
#import "MapViewController.h"
#import "Statistics.h"

#include "Framework.h"

#include "storage/country_info_getter.hpp"

#include "platform/local_country_file_utils.hpp"

namespace
{
CGSize constexpr kInitialDialogSize = {200, 200};

BOOL canAutoDownload(storage::TCountryId const & countryId)
{
  if (![MWMSettings autoDownloadEnabled])
    return NO;
  if (GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_WIFI)
    return NO;
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return NO;
  auto const & countryInfoGetter = GetFramework().GetCountryInfoGetter();
  if (countryId != countryInfoGetter.GetRegionCountryId(lastLocation.mercator))
    return NO;
  return !platform::migrate::NeedMigrate();
}
}  // namespace

using namespace storage;

@interface MWMMapDownloadDialog ()<MWMFrameworkStorageObserver, MWMCircularProgressProtocol>
@property(weak, nonatomic) IBOutlet UILabel * parentNode;
@property(weak, nonatomic) IBOutlet UILabel * node;
@property(weak, nonatomic) IBOutlet UILabel * nodeSize;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * nodeTopOffset;
@property(weak, nonatomic) IBOutlet UIButton * downloadButton;
@property(weak, nonatomic) IBOutlet UIView * progressWrapper;

@property(weak, nonatomic) MapViewController * controller;

@property(nonatomic) MWMCircularProgress * progress;

@property(nonatomic) NSMutableArray<NSDate *> * skipDownloadTimes;

@property(nonatomic) BOOL isAutoDownloadCancelled;

@end

@implementation MWMMapDownloadDialog
{
  TCountryId m_countryId;
  TCountryId m_autoDownloadCountryId;
}

+ (instancetype)dialogForController:(MapViewController *)controller
{
  MWMMapDownloadDialog * dialog =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  dialog.autoresizingMask = UIViewAutoresizingFlexibleHeight;
  dialog.controller = controller;
  dialog.size = kInitialDialogSize;
  return dialog;
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.center = {superview.midX, superview.midY};
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     CGSize const newSize =
                         [self systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
                     if (CGSizeEqualToSize(newSize, self.size))
                       return;
                     self.size = newSize;
                     self.center = {superview.midX, superview.midY};
                     [self layoutIfNeeded];
                   }];
  [super layoutSubviews];
}

- (void)configDialog
{
  auto & f = GetFramework();
  auto const & s = f.GetStorage();
  auto const & p = f.GetDownloadingPolicy();

  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(m_countryId, nodeAttrs);

  if (!nodeAttrs.m_present && ![MWMRouter isRoutingActive])
  {
    BOOL const isMultiParent = nodeAttrs.m_parentInfo.size() > 1;
    BOOL const noParrent = (nodeAttrs.m_parentInfo[0].m_id == s.GetRootId());
    BOOL const hideParent = (noParrent || isMultiParent);
    self.parentNode.hidden = hideParent;
    self.nodeTopOffset.priority =
        hideParent ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
    if (!hideParent)
    {
      self.parentNode.text = @(nodeAttrs.m_topmostParentInfo[0].m_localName.c_str());
      self.parentNode.textColor = [UIColor blackSecondaryText];
    }
    self.node.text = @(nodeAttrs.m_nodeLocalName.c_str());
    self.node.textColor = [UIColor blackPrimaryText];
    self.nodeSize.hidden = platform::migrate::NeedMigrate();
    self.nodeSize.textColor = [UIColor blackSecondaryText];
    self.nodeSize.text = formattedSize(nodeAttrs.m_mwmSize);

    switch (nodeAttrs.m_status)
    {
    case NodeStatus::NotDownloaded:
    case NodeStatus::Partly:
    {
      MapViewController * controller = self.controller;
      BOOL const isMapVisible =
          [controller.navigationController.topViewController isEqual:controller];
      if (isMapVisible && !self.isAutoDownloadCancelled && canAutoDownload(m_countryId))
      {
        [Statistics logEvent:kStatDownloaderMapAction
              withParameters:@{
                kStatAction: kStatDownload,
                kStatIsAuto: kStatYes,
                kStatFrom: kStatMap,
                kStatScenario: kStatDownload
              }];
        m_autoDownloadCountryId = m_countryId;
        [MWMStorage downloadNode:m_countryId
                       onSuccess:^{
                         [self showInQueue];
                       }];
      }
      else
      {
        m_autoDownloadCountryId = kInvalidCountryId;
        [self showDownloadRequest];
      }
      break;
      }
      case NodeStatus::Downloading:
        if (nodeAttrs.m_downloadingProgress.second != 0)
          [self showDownloading:static_cast<CGFloat>(nodeAttrs.m_downloadingProgress.first) /
                                nodeAttrs.m_downloadingProgress.second];
        break;
      case NodeStatus::Applying:
      case NodeStatus::InQueue: [self showInQueue]; break;
      case NodeStatus::Undefined:
      case NodeStatus::Error:
        if (p.IsAutoRetryDownloadFailed())
          [self showError:nodeAttrs.m_error];
        break;
      case NodeStatus::OnDisk:
      case NodeStatus::OnDiskOutOfDate: [self removeFromSuperview]; break;
    }
  }
  else
  {
    [self removeFromSuperview];
  }

  if (self.superview)
    [self setNeedsLayout];
}

- (void)addToSuperview
{
  if (self.superview)
    return;
  auto superview = self.controller.view;
  auto bottomMenuView = [MWMBottomMenuViewController controller].view;
  if (bottomMenuView)
    [superview insertSubview:self belowSubview:bottomMenuView];
  else
    [superview addSubview:self];
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
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.state = MWMCircularProgressStateFailed;
  MWMAlertViewController * avc = self.controller.alertController;
  [self addToSuperview];
  auto const retryBlock = ^{
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction: kStatRetry,
            kStatIsAuto: kStatNo,
            kStatFrom: kStatMap,
            kStatScenario: kStatDownload
          }];
    [self showInQueue];
    [MWMStorage retryDownloadNode:self->m_countryId];
  };
  auto const cancelBlock = ^{
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatMap}];
    [MWMStorage cancelDownloadNode:self->m_countryId];
  };
  switch (errorCode)
  {
  case NodeErrorCode::NoError: break;
  case NodeErrorCode::UnknownError:
    [avc presentDownloaderInternalErrorAlertWithOkBlock:retryBlock cancelBlock:cancelBlock];
    break;
  case NodeErrorCode::OutOfMemFailed: [avc presentDownloaderNotEnoughSpaceAlert]; break;
  case NodeErrorCode::NoInetConnection:
    [avc presentDownloaderNoConnectionAlertWithOkBlock:retryBlock cancelBlock:cancelBlock];
    break;
  }
}

- (void)showDownloadRequest
{
  self.downloadButton.hidden = NO;
  self.progressWrapper.hidden = YES;
  [self addToSuperview];
}

- (void)showDownloading:(CGFloat)progress
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = [NSString stringWithFormat:@"%@ %@%%", L(@"downloader_downloading"),
                                                  @(static_cast<NSUInteger>(progress * 100))];
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.progress = progress;
  [self addToSuperview];
}

- (void)showInQueue
{
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = L(@"downloader_queued");
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.state = MWMCircularProgressStateSpinner;
  [self addToSuperview];
}

- (void)processViewportCountryEvent:(TCountryId const &)countryId
{
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
  if (self.superview)
    [self configDialog];
  else
    [self removeFromSuperview];
}

- (void)processCountry:(TCountryId const &)countryId
              progress:(MapFilesDownloader::TProgress const &)progress
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
            kStatAction: kStatRetry,
            kStatIsAuto: kStatNo,
            kStatFrom: kStatMap,
            kStatScenario: kStatDownload
          }];
    [self showInQueue];
    [MWMStorage retryDownloadNode:m_countryId];
  }
  else
  {
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatMap}];
    if (m_autoDownloadCountryId == m_countryId)
      self.isAutoDownloadCancelled = YES;
    [MWMStorage cancelDownloadNode:m_countryId];
  }
}

#pragma mark - Actions

- (IBAction)downloadAction
{
  MapViewController * controller = self.controller;
  if (platform::migrate::NeedMigrate())
  {
    [Statistics logEvent:kStatDownloaderMigrationDialogue withParameters:@{kStatFrom : kStatMap}];
    [controller openMigration];
  }
  else
  {
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction: kStatDownload,
            kStatIsAuto: kStatNo,
            kStatFrom: kStatMap,
            kStatScenario: kStatDownload
          }];
    [MWMStorage downloadNode:m_countryId
                   onSuccess:^{
                     [self showInQueue];
                   }];
  }
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
