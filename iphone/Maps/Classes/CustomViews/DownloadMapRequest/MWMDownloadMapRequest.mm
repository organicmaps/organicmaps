#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMCircularProgress.h"
#import "MWMDownloadMapRequest.h"
#import "MWMDownloadMapRequestView.h"
#import "MWMMapDownloadDialog.h"
#import "MWMStorage.h"
#import "Statistics.h"

#include "Framework.h"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"

@interface MWMDownloadMapRequest () <MWMCircularProgressProtocol>

@property (nonatomic) IBOutlet MWMDownloadMapRequestView * rootView;
@property (nonatomic) IBOutlet UILabel * mapTitleLabel;
@property (nonatomic) IBOutlet UIButton * downloadMapButton;
@property (nonatomic) IBOutlet UIView * progressViewWrapper;

@property (nonatomic) MWMCircularProgress * progress;

@property (copy, nonatomic) NSString * mapAndRouteSize;

@property (weak, nonatomic) id <MWMDownloadMapRequestProtocol> delegate;

@end

@implementation MWMDownloadMapRequest
{
  storage::TCountryId m_countryId;
}

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView
                                  delegate:(nonnull id <MWMDownloadMapRequestProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:self.class.className owner:self options:nil];
    [parentView addSubview:self.rootView];
    self.delegate = delegate;
    [self showRequest];
  }
  return self;
}

- (void)dealloc
{
  [self.rootView removeFromSuperview];
}

- (void)showRequest
{
  auto & f = GetFramework();
  auto & s = f.Storage();
  if (s.IsDownloadInProgress())
  {
    m_countryId = s.GetCurrentDownloadingCountryId();
    [self updateState:MWMDownloadMapRequestStateDownload];
  }
  else
  {
    m_countryId = storage::kInvalidCountryId;
    auto const & countryInfoGetter = f.CountryInfoGetter();
    LocationManager * locationManager = [MapsAppDelegate theApp].locationManager;
    if (locationManager.lastLocationIsValid)
      m_countryId = countryInfoGetter.GetRegionCountryId(locationManager.lastLocation.mercator);

    if (m_countryId != storage::kInvalidCountryId)
    {
      storage::NodeAttrs attrs;
      s.GetNodeAttrs(m_countryId, attrs);
      self.mapTitleLabel.text = @(attrs.m_nodeLocalName.c_str());
      self.mapAndRouteSize = formattedSize(attrs.m_mwmSize);
      [self.downloadMapButton setTitle:[NSString stringWithFormat:@"%@ (%@)",
                                        L(@"downloader_download_map"), self.mapAndRouteSize]
                              forState:UIControlStateNormal];
      [self updateState:MWMDownloadMapRequestStateRequestLocation];
    }
    else
    {
      [self updateState:MWMDownloadMapRequestStateRequestUnknownLocation];
    }
  }
  self.rootView.hidden = NO;
}

#pragma mark - Process control

- (void)downloadProgress:(CGFloat)progress
{
  [self showRequest];
  self.progress.progress = progress;
}

- (void)setDownloadFailed
{
  self.progress.state = MWMCircularProgressStateFailed;
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
            kStatFrom : kStatSearch,
            kStatScenario : kStatDownload
          }];
    [MWMStorage retryDownloadNode:m_countryId];
    self.progress.state = MWMCircularProgressStateSpinner;
  }
  else
  {
    [Statistics logEvent:kStatDownloaderDownloadCancel withParameters:@{kStatFrom : kStatSearch}];
    [MWMStorage cancelDownloadNode:m_countryId];
  }
  [self showRequest];
}

#pragma mark - Actions

- (IBAction)downloadMapTouchUpInside:(nonnull UIButton *)sender
{
  [Statistics logEvent:kStatDownloaderMapAction
        withParameters:@{
          kStatAction : kStatDownload,
          kStatIsAuto : kStatNo,
          kStatFrom : kStatSearch,
          kStatScenario : kStatDownload
        }];
  [MWMStorage downloadNode:m_countryId alertController:self.delegate.alertController onSuccess:^
  {
    [self showRequest];
    self.progress.state = MWMCircularProgressStateSpinner;
  }];
}

- (IBAction)selectMapTouchUpInside:(nonnull UIButton *)sender
{
  [Statistics logEvent:kStatEventName(kStatDownloadRequest, kStatSelectMap)];
  [self.delegate selectMapsAction];
}

- (void)updateState:(enum MWMDownloadMapRequestState)state
{
  self.rootView.state = state;
  [self.delegate stateUpdated:state];
}

- (MWMCircularProgress *)progress
{
  if (!_progress)
  {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.progressViewWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

@end
