#import "MWMMapDownloadDialog.h"
#import <SafariServices/SafariServices.h>
#import "CLLocation+Mercator.h"
#import "MWMBannerHelpers.h"
#import "MWMCircularProgress.h"
#import "MWMStorage+UI.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "partners_api/ads/ads_engine.hpp"
#include "partners_api/ads/banner.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/network_policy.hpp"
#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"

namespace {
CGSize constexpr kInitialDialogSize = {200, 200};

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

ads::Banner getPromoBanner(std::string const &mwmId) {
  auto const pos = GetFramework().GetCurrentPosition();
  auto const banners =
      GetFramework().GetAdsEngine().GetDownloadOnMapBanners(mwmId, pos, languages::GetCurrentNorm());

  if (banners.empty())
    return {};

  return banners[0];
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
@property(strong, nonatomic) IBOutlet UIView *bannerView;
@property(strong, nonatomic) IBOutlet UIView *bannerContentView;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *bannerVisibleConstraintV;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *bannerVisibleConstraintH;

@property(weak, nonatomic) MapViewController *controller;
@property(nonatomic) MWMCircularProgress *progress;
@property(nonatomic) NSMutableArray<NSDate *> *skipDownloadTimes;
@property(nonatomic) BOOL isAutoDownloadCancelled;
@property(strong, nonatomic) UIViewController *bannerViewController;

@end

@implementation MWMMapDownloadDialog {
  CountryId m_countryId;
  CountryId m_autoDownloadCountryId;
  ads::Banner m_promoBanner;
}

+ (instancetype)dialogForController:(MapViewController *)controller {
  MWMMapDownloadDialog *dialog = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  dialog.autoresizingMask = UIViewAutoresizingFlexibleHeight;
  dialog.controller = controller;
  dialog.size = kInitialDialogSize;
  return dialog;
}

- (void)layoutSubviews {
  UIView *superview = self.superview;
  self.center = {superview.midX, superview.midY};
  CGSize const newSize = [self systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  if (CGSizeEqualToSize(newSize, self.size))
    return;
  self.size = newSize;
  self.center = {superview.midX, superview.midY};
  [super layoutSubviews];
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
        [self removePreviousBunnerIfNeeded];
        MapViewController *controller = self.controller;
        BOOL const isMapVisible = [controller.navigationController.topViewController isEqual:controller];
        if (isMapVisible && !self.isAutoDownloadCancelled && canAutoDownload(m_countryId)) {
          [Statistics logEvent:kStatDownloaderMapAction
                withParameters:@{
                  kStatAction: kStatDownload,
                  kStatIsAuto: kStatYes,
                  kStatFrom: kStatMap,
                  kStatScenario: kStatDownload
                }];
          m_autoDownloadCountryId = m_countryId;
          [[MWMStorage sharedStorage] downloadNode:@(m_countryId.c_str())
                                         onSuccess:^{
                                                      [self showInQueue];
                                                    }];
        } else {
          m_autoDownloadCountryId = kInvalidCountryId;
          [self showDownloadRequest];
        }
        if (@available(iOS 12.0, *)) {
          [[MWMCarPlayService shared] showNoMapAlert];
        }
        break;
      }
      case NodeStatus::Downloading:
        if (nodeAttrs.m_downloadingProgress.m_bytesTotal != 0)
          [self showDownloading:(CGFloat)nodeAttrs.m_downloadingProgress.m_bytesDownloaded /
                                nodeAttrs.m_downloadingProgress.m_bytesTotal];
        [self showBannerIfNeeded];
        break;
      case NodeStatus::Applying:
      case NodeStatus::InQueue:
        [self showInQueue];
        break;
      case NodeStatus::Undefined:
      case NodeStatus::Error:
        if (p.IsAutoRetryDownloadFailed()) {
          [self removePreviousBunnerIfNeeded];
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
}

- (void)removeFromSuperview {
  if (@available(iOS 12.0, *)) {
    [[MWMCarPlayService shared] hideNoMapAlert];
  }
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
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction: kStatRetry,
            kStatIsAuto: kStatNo,
            kStatFrom: kStatMap,
            kStatScenario: kStatDownload
          }];
    [self showInQueue];
    [[MWMStorage sharedStorage] retryDownloadNode:@(self->m_countryId.c_str())];
  };
  auto const cancelBlock = ^{
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom: kStatMap}];
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
  [self hideBanner];
  self.downloadButton.hidden = NO;
  self.progressWrapper.hidden = YES;
  [self addToSuperview];
}

- (void)showDownloading:(CGFloat)progress {
  self.nodeSize.textColor = [UIColor blackSecondaryText];
  self.nodeSize.text =
    [NSString stringWithFormat:@"%@ %@%%", L(@"downloader_downloading"), @((NSInteger)(progress * 100.f))];
  self.downloadButton.hidden = YES;
  self.progressWrapper.hidden = NO;
  self.progress.progress = progress;
  [self addToSuperview];
}

- (void)showInQueue {
  [self showBannerIfNeeded];
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

- (NSString *)getStatProvider:(MWMBannerType)bannerType {
  switch (bannerType) {
  case MWMBannerTypeTinkoffAllAirlines: return kStatTinkoffAirlines;
  case MWMBannerTypeTinkoffInsurance: return kStatTinkoffInsurance;
  case MWMBannerTypeMts: return kStatMts;
  case MWMBannerTypeSkyeng: return kStatSkyeng;
  case MWMBannerTypeBookmarkCatalog: return kStatMapsmeGuides;
  case MWMBannerTypeMastercardSberbank: return kStatMastercardSberbank;
  case MWMBannerTypeArsenalMedic: return kStatArsenalMedic;
  case MWMBannerTypeArsenalFlat: return kStatArsenalFlat;
  case MWMBannerTypeArsenalInsuranceCrimea: return kStatArsenalInsuranceCrimea;
  case MWMBannerTypeArsenalInsuranceRussia: return kStatArsenalInsuranceRussia;
  case MWMBannerTypeArsenalInsuranceWorld: return kStatArsenalInsuranceWorld;
  default: return @("");
  }
}

- (void)showBannerIfNeeded {
  m_promoBanner = getPromoBanner(m_countryId);
  [self layoutIfNeeded];
  if (self.bannerView.hidden) {
    MWMBannerType bannerType = banner_helpers::MatchBannerType(m_promoBanner.m_type);
    NSString *statProvider = [self getStatProvider:bannerType];
    switch (bannerType) {
      case MWMBannerTypeTinkoffAllAirlines:
      case MWMBannerTypeTinkoffInsurance:
      case MWMBannerTypeMts:
      case MWMBannerTypeSkyeng:
      case MWMBannerTypeMastercardSberbank:
      case MWMBannerTypeArsenalMedic:
      case MWMBannerTypeArsenalFlat:
      case MWMBannerTypeArsenalInsuranceCrimea:
      case MWMBannerTypeArsenalInsuranceRussia:
      case MWMBannerTypeArsenalInsuranceWorld: {
        __weak __typeof(self) ws = self;
        MWMVoidBlock onClick = ^{
          [ws bannerAction];
          [Statistics logEvent:kStatDownloaderBannerClick
                withParameters:@{
                  kStatFrom: kStatMap,
                  kStatProvider: statProvider,
                  kStatMWMName: @(self->m_countryId.c_str())
                }];
        };

        UIViewController *controller = [PartnerBannerBuilder buildWithType:bannerType tapHandler:onClick];
        self.bannerViewController = controller;
        break;
      }
      case MWMBannerTypeBookmarkCatalog: {
        __weak __typeof(self) ws = self;
        self.bannerViewController = [[BookmarksBannerViewController alloc] initWithTapHandler:^{
          __strong __typeof(self) self = ws;
          NSString *urlString = @(self->m_promoBanner.m_value.c_str());
          if (urlString.length == 0) {
            return;
          }
          NSURL *url = [NSURL URLWithString:urlString];
          [self.controller openCatalogAbsoluteUrl:url animated:YES utm:MWMUTMDownloadMwmBanner];
          [Statistics logEvent:kStatDownloaderBannerClick
                withParameters:@{
                                 kStatFrom: kStatMap,
                                 kStatProvider: statProvider
                                 }];
        }];
        break;
      }
      default:
        self.bannerViewController = nil;
        break;
    }

    if (self.bannerViewController) {
      UIView *bannerView = self.bannerViewController.view;
      [self.bannerContentView addSubview:bannerView];
      bannerView.translatesAutoresizingMaskIntoConstraints = NO;
      [NSLayoutConstraint activateConstraints:@[
        [bannerView.topAnchor constraintEqualToAnchor:self.bannerContentView.topAnchor],
        [bannerView.leftAnchor constraintEqualToAnchor:self.bannerContentView.leftAnchor],
        [bannerView.bottomAnchor constraintEqualToAnchor:self.bannerContentView.bottomAnchor],
        [bannerView.rightAnchor constraintEqualToAnchor:self.bannerContentView.rightAnchor]
      ]];
      self.bannerVisibleConstraintV.priority = UILayoutPriorityDefaultHigh;
      self.bannerVisibleConstraintH.priority = UILayoutPriorityDefaultHigh;
      self.bannerView.hidden = NO;
      self.bannerView.alpha = 0;
      [UIView animateWithDuration:kDefaultAnimationDuration
                       animations:^{
                         self.bannerView.alpha = 1;
                         [self layoutIfNeeded];
                       }];
      [Statistics
              logEvent:kStatDownloaderBannerShow
        withParameters:@{kStatFrom: kStatMap, kStatProvider: statProvider, kStatMWMName: @(self->m_countryId.c_str())}];
    }
  }
}

- (void)hideBanner {
  [self layoutIfNeeded];
  self.bannerVisibleConstraintV.priority = UILayoutPriorityDefaultLow;
  self.bannerVisibleConstraintH.priority = UILayoutPriorityDefaultLow;
  [UIView animateWithDuration:kDefaultAnimationDuration
    animations:^{
      [self layoutIfNeeded];
      self.bannerView.alpha = 0;
    }
    completion:^(BOOL finished) {
      [self.bannerViewController.view removeFromSuperview];
      self.bannerViewController = nil;
      self.bannerView.hidden = YES;
    }];
}

- (void)removePreviousBunnerIfNeeded {
  if (!self.bannerViewController) {
    return;
  }
  self.bannerVisibleConstraintV.priority = UILayoutPriorityDefaultLow;
  self.bannerVisibleConstraintH.priority = UILayoutPriorityDefaultLow;
  [self.bannerViewController.view removeFromSuperview];
  self.bannerViewController = nil;
  self.bannerView.hidden = YES;
  [self layoutIfNeeded];
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
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction: kStatRetry,
            kStatIsAuto: kStatNo,
            kStatFrom: kStatMap,
            kStatScenario: kStatDownload
          }];
    [self showInQueue];
    [[MWMStorage sharedStorage] retryDownloadNode:@(m_countryId.c_str())];
  } else {
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom: kStatMap}];
    if (m_autoDownloadCountryId == m_countryId)
      self.isAutoDownloadCancelled = YES;
    [[MWMStorage sharedStorage] cancelDownloadNode:@(m_countryId.c_str())];
  }
}

#pragma mark - Actions

- (IBAction)bannerAction {
  if (m_promoBanner.m_value.empty())
    return;

  NSURL *bannerURL = [NSURL URLWithString:@(m_promoBanner.m_value.c_str())];
  SFSafariViewController *safari = [[SFSafariViewController alloc] initWithURL:bannerURL];
  [self.controller presentViewController:safari animated:YES completion:nil];
}

- (IBAction)downloadAction {
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction: kStatDownload,
          kStatIsAuto: kStatNo,
          kStatFrom: kStatMap,
          kStatScenario: kStatDownload
        }];
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
