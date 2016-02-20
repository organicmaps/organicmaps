#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMCircularProgress.h"
#import "MWMDownloadMapRequest.h"
#import "MWMDownloadMapRequestView.h"
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

@property (nonatomic) MWMCircularProgress * progressView;

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
    [self setupProgressView];
    self.delegate = delegate;
    [self showRequest];
  }
  return self;
}

- (void)setupProgressView
{
  self.progressView = [[MWMCircularProgress alloc] initWithParentView:self.progressViewWrapper];
  self.progressView.delegate = self;
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateNormal];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateSelected];
  [self.progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
  [self.progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateSpinner];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
  [self.progressView setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
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
    self.progressView.state = MWMCircularProgressStateProgress;
    [self updateState:MWMDownloadMapRequestStateDownload];
  }
  else
  {
    m_countryId = storage::kInvalidCountryId;
    auto const & countryInfoGetter = f.CountryInfoGetter();
    LocationManager * locationManager = [MapsAppDelegate theApp].m_locationManager;
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

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName
{
  self.progressView.progress = progress;
  [self showRequest];
  self.mapTitleLabel.text = countryName;
}

- (void)setDownloadFailed
{
  self.progressView.state = MWMCircularProgressStateFailed;
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  [[Statistics instance] logEvent:kStatEventName(kStatDownloadRequest, kStatButton)
                   withParameters:@{kStatValue : kStatProgress}];
  if (progress.state == MWMCircularProgressStateFailed)
  {
    [MWMStorage retryDownloadNode:m_countryId];
    self.progressView.state = MWMCircularProgressStateSpinner;
  }
  else
  {
    [MWMStorage cancelDownloadNode:m_countryId];
  }
  [self showRequest];
}

#pragma mark - Actions

- (IBAction)downloadMapTouchUpInside:(nonnull UIButton *)sender
{
  [[Statistics instance] logEvent:kStatEventName(kStatDownloadRequest, kStatDownloadMap)];
  [MWMStorage downloadNode:m_countryId alertController:self.delegate.alertController onSuccess:^
  {
    [self showRequest];
    self.progressView.state = MWMCircularProgressStateSpinner;
  }];
}

- (IBAction)selectMapTouchUpInside:(nonnull UIButton *)sender
{
  [[Statistics instance] logEvent:kStatEventName(kStatDownloadRequest, kStatSelectMap)];
  [self.delegate selectMapsAction];
}

- (void)updateState:(enum MWMDownloadMapRequestState)state
{
  self.rootView.state = state;
  [self.delegate stateUpdated:state];
}

@end
