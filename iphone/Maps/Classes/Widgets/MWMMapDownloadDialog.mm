#import "MWMMapDownloadDialog.h"
#import <SafariServices/SafariServices.h>
#import "CLLocation+Mercator.h"
#import "MWMCircularProgress.h"
#import "MWMStorage+UI.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "storage/country_info_getter.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/network_policy.hpp"
#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"

namespace {

BOOL canAutoDownload(storage::CountryId const &countryId) {
  if (![MWMSettings autoDownloadEnabled])
    return NO;
  if (GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_WIFI)
    return NO;
  CLLocation *lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return NO;
  auto const &countryInfoGetter = GetFramework().GetCountryInfoGetter();
  if (countryId != countryInfoGetter.GetRegionCountryId(lastLocation.mercator))
    return NO;
  return YES;
}
}  // namespace

using namespace storage;

@interface MWMMapDownloadDialog () <MWMStorageObserver, MWMCircularProgressProtocol>
@property(strong, nonatomic) IBOutlet UILabel *parentNode;
@property(strong, nonatomic) IBOutlet UILabel *node;
@property(strong, nonatomic) IBOutlet UILabel *nodeSize;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *nodeTopOffset;
@property(strong, nonatomic) IBOutlet UIButton *downloadButton;
@property(strong, nonatomic) IBOutlet UIView *progressWrapper;

@property(weak, nonatomic) MapViewController *controller;
@property(nonatomic) MWMCircularProgress *progress;
@property(nonatomic) NSMutableArray<NSDate *> *skipDownloadTimes;
@property(nonatomic) BOOL isAutoDownloadCancelled;

@end

@implementation MWMMapDownloadDialog {
  CountryId m_countryId;
  CountryId m_autoDownloadCountryId;
}

+ (instancetype)dialogForController:(MapViewController *)controller {
  MWMMapDownloadDialog *dialog = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  dialog.controller = controller;
  return dialog;
}

- (void)configDialog {
  auto &f = GetFramework();
  auto const &s = f.GetStorage();
  auto const &p = f.GetDownloadingPolicy();

  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(m_countryId, nodeAttrs);

  if (!nodeAttrs.m_present && ![MWMRouter isRoutingActive]) {
    BOOL const isMultiParent = nodeAttrs.m_parentInfo.size() > 1;
    BOOL const noParrent = (nodeAttrs.m_parentInfo[0].m_id == s.GetRootId());
    BOOL const hideParent = (noParrent || isMultiParent);
    self.parentNode.hidden = hideParent;
    self.nodeTopOffset.priority = hideParent ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
    if (!hideParent) {
      self.parentNode.text = @(nodeAttrs.m_topmostParentInfo[0].m_localName.c_str());
      self.parentNode.textColor = [UIColor blackSecondaryText];
    }
    self.node.text = @(nodeAttrs.m_nodeLocalName.c_str());
    self.node.textColor = [UIColor blackPrimaryText];
    self.nodeSize.hidden = NO;
    self.nodeSize.textColor = [UIColor blackSecondaryText];
    self.nodeSize.text = formattedSize(nodeAttrs.m_mwmSize);

    switch (nodeAttrs.m_status) {
      case NodeStatus::NotDownloaded:
      case NodeStatus::Partly: {
        MapViewController *controller = self.controller;
        BOOL const isMapVisible = [controller.navigationController.topViewController isEqual:controller];
        if (isMapVisible && !self.isAutoDownloadCancelled && canAutoDownload(m_countryId)) {
          m_autoDownloadCountryId = m_countryId;
          [[MWMStorage sharedStorage] downloadNode:@(m_countryId.c_str())
                                         onSuccess:^{
                                                      [self showInQueue];
                                                    }];
        } else {
          m_autoDownloadCountryId = kInvalidCountryId;
          [self showDownloadRequest];
        }
        [[MWMCarPlayService shared] showNoMapAlert];
        break;
      }
      case NodeStatus::Downloading:
        if (nodeAttrs.m_downloadingProgress.m_bytesTotal != 0)
          [self showDownloading:(CGFloat)nodeAttrs.m_downloadingProgress.m_bytesDownloaded /
                                nodeAttrs.m_downloadingProgress.m_bytesTotal];
        break;
      case NodeStatus::Applying:
      case NodeStatus::InQueue:
        [self showInQueue];
        break;
      case NodeStatus::Undefined:
      case NodeStatus::Error:
        if (p.IsAutoRetryDownloadFailed()) {
          [self showError:nodeAttrs.m_error];
        } else {
          [self showInQueue];
        }
        break;
      case NodeStatus::OnDisk:
      case NodeStatus::OnDiskOutOfDate:
        [self removeFromSuperview];
        break;
    }
  } else {
    [self removeFromSuperview];
  }

  if (self.superview)
    [self setNeedsLayout];
}

- (void)addToSuperview {
  if (self.superview)
    return;
  MapViewController *controller = self.controller;
  [controller.view insertSubview:self aboveSubview:controller.controlsView];
  [[MWMStorage sharedStorage] addObserver:self];

  // Center dialog in the parent view.
  [self.centerXAnchor constraintEqualToAnchor:controller.view.centerXAnchor].active = YES;
  [self.centerYAnchor constraintEqualToAnchor:controller.view.centerYAnchor].active = YES;
}


- (void)removeFromSuperview {
  [[MWMCarPlayService shared] hideNoMapAlert];
  self.progress.state = MWMCircularProgressStateNormal;
  [[MWMStorage sharedStorage] removeObserver:self];
  [super removeFromSuperview];
}

- (void)showError:(NodeErrorCode)errorCode {
  if (errorCode == NodeErrorCode::NoError)
    return;
  self.nodeSize.textColor = [UIColor red];
  self.nodeSize.text = L(@"country_status_download_failed");
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.state = MWMCircularProgressStateFailed;
  MWMAlertViewController *avc = self.controller.alertController;
  [self addToSuperview];
  auto const retryBlock = ^{
    [self showInQueue];
    [[MWMStorage sharedStorage] retryDownloadNode:@(self->m_countryId.c_str())];
  };
  auto const cancelBlock = ^{
    [[MWMStorage sharedStorage] cancelDownloadNode:@(self->m_countryId.c_str())];
  };
  switch (errorCode) {
    case NodeErrorCode::NoError:
      break;
    case NodeErrorCode::UnknownError:
      [avc presentDownloaderInternalErrorAlertWithOkBlock:retryBlock cancelBlock:cancelBlock];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [avc presentDownloaderNotEnoughSpaceAlert];
      break;
    case NodeErrorCode::NoInetConnection:
      [avc presentDownloaderNoConnectionAlertWithOkBlock:retryBlock cancelBlock:cancelBlock];
      break;
  }
}

- (void)showDownloadRequest {
  self.downloadButton.hidden = NO;
  self.progressWrapper.hidden = YES;
  [self addToSuperview];
}

- (void)showDownloading:(CGFloat)progress {
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text =
    [NSString stringWithFormat:@"%@ %.2f%%", L(@"downloader_downloading"), progress * 100.f];
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.progress = progress;
  [self addToSuperview];
}

- (void)showInQueue {
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text = L(@"downloader_queued");
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.state = MWMCircularProgressStateSpinner;
  [self addToSuperview];
}

- (void)processViewportCountryEvent:(CountryId const &)countryId {
  m_countryId = countryId;
  if (countryId == kInvalidCountryId)
    [self removeFromSuperview];
  else
    [self configDialog];
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId {
  if (m_countryId != countryId.UTF8String)
    return;
  if (self.superview)
    [self configDialog];
  else
    [self removeFromSuperview];
}

- (void)processCountry:(NSString *)countryId
       downloadedBytes:(uint64_t)downloadedBytes
            totalBytes:(uint64_t)totalBytes {
  if (self.superview && m_countryId == countryId.UTF8String)
    [self showDownloading:(CGFloat)downloadedBytes / totalBytes];
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress {
  if (progress.state == MWMCircularProgressStateFailed) {
    [self showInQueue];
    [[MWMStorage sharedStorage] retryDownloadNode:@(m_countryId.c_str())];
  } else {
    if (m_autoDownloadCountryId == m_countryId)
      self.isAutoDownloadCancelled = YES;
    [[MWMStorage sharedStorage] cancelDownloadNode:@(m_countryId.c_str())];
  }
}

#pragma mark - Actions

- (IBAction)downloadAction {
  [[MWMStorage sharedStorage] downloadNode:@(m_countryId.c_str())
                                 onSuccess:^{ [self showInQueue]; }];
}

#pragma mark - Properties

- (MWMCircularProgress *)progress {
  if (!_progress) {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.progressWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

- (NSMutableArray<NSDate *> *)skipDownloadTimes {
  if (!_skipDownloadTimes)
    _skipDownloadTimes = [@[] mutableCopy];
  return _skipDownloadTimes;
}

@end
