#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMMigrationView.h"
#import "MWMMigrationViewController.h"
#import "MWMStorage.h"
#import "Statistics.h"

#include "Framework.h"

#include "platform/platform.hpp"
#include "storage/storage.hpp"

namespace
{
NSString * const kDownloaderSegue = @"Migration2MapDownloaderSegue";
} // namespace

using namespace storage;

@implementation MWMMigrationViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self checkState];
}

- (void)configNavBar
{
  self.title = L(@"download_maps");
}

- (void)checkState
{
  if (!GetFramework().IsEnoughSpaceForMigrate())
    [self setState:MWMMigrationViewState::ErrorNoSpace];
  else if (!Platform::IsConnected())
    [self setState:MWMMigrationViewState::ErrorNoConnection];
  else
    [self setState:MWMMigrationViewState::Default];
}

- (void)performLimitedMigration:(BOOL)limited
{
  [Statistics logEvent:kStatDownloaderMigrationStarted
        withParameters:@{kStatType : limited ? kStatCurrentMap : kStatAllMaps}];
  auto & f = GetFramework();
  LocationManager * lm = [MapsAppDelegate theApp].m_locationManager;
  ms::LatLon position{};
  if (![lm getLat:position.lat Lon:position.lon])
    position = MercatorBounds::ToLatLon(f.GetViewportCenter());

  auto migrate = ^
  {
    GetFramework().Migrate(!limited);
    [self performSegueWithIdentifier:kDownloaderSegue sender:self];
    [Statistics logEvent:kStatDownloaderMigrationCompleted];
  };

  auto onStatusChanged = [self, migrate](TCountryId const & countryId)
  {
    auto & f = GetFramework();
    auto s = f.Storage().GetPrefetchStorage();
    NodeAttrs nodeAttrs;
    s->GetNodeAttrs(countryId, nodeAttrs);
    switch (nodeAttrs.m_status)
    {
      case NodeStatus::OnDisk:
        migrate();
        break;
      case NodeStatus::Undefined:
      case NodeStatus::Error:
        [self showError:nodeAttrs.m_error countryId:countryId]; break;
      default:
        break;
    }
  };

  auto onProgressChanged = [](TCountryId const & countryId, TLocalAndRemoteSize const & progress){};

  if (f.PreMigrate(position, onStatusChanged, onProgressChanged))
    [self setState:MWMMigrationViewState::Processing];
  else
    migrate();
}

- (void)showError:(NodeErrorCode)errorCode countryId:(TCountryId const &)countryId
{
  MWMAlertViewController * avc = self.alertController;
  switch (errorCode)
  {
    case NodeErrorCode::NoError:
      break;
    case NodeErrorCode::UnknownError:
      [Statistics logEvent:[NSString stringWithFormat:@"%@%@", kStatDownloaderError, kStatUnknownError]];
      [avc presentInternalErrorAlert];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [Statistics logEvent:[NSString stringWithFormat:@"%@%@", kStatDownloaderError, kStatNotEnoughSpaceError]];
      [avc presentDownloaderNotEnoughSpaceAlert];
      break;
    case NodeErrorCode::NoInetConnection:
      [Statistics logEvent:[NSString stringWithFormat:@"%@%@", kStatDownloaderError, kStatNetworkError]];
      [avc presentDownloaderNoConnectionAlertWithOkBlock:^
      {
        [MWMStorage retryDownloadNode:countryId];
      }];
      break;
  }
}

- (void)setState:(MWMMigrationViewState)state
{
  static_cast<MWMMigrationView *>(self.view).state = state;
  self.navigationItem.leftBarButtonItem.enabled = (state != MWMMigrationViewState::Processing);
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  if ([segue.identifier isEqualToString:kDownloaderSegue])
  {
    MWMMapDownloaderViewController * dvc = segue.destinationViewController;
    dvc.parentCountryId = GetFramework().Storage().GetRootId();
  }
}

#pragma mark - Actions

- (IBAction)primaryAction
{
  [self performLimitedMigration:NO];
}

- (IBAction)secondaryAction
{
  [self performLimitedMigration:YES];
}

@end
