#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMMigrationView.h"
#import "MWMMigrationViewController.h"
#import "MWMStorage.h"

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
  MWMMigrationView * mv = static_cast<MWMMigrationView *>(self.view);
  if (!GetFramework().IsEnoughSpaceForMigrate())
    mv.state = MWMMigrationViewState::ErrorNoSpace;
  else if (!Platform::IsConnected())
    mv.state = MWMMigrationViewState::ErrorNoConnection;
  else
    mv.state = MWMMigrationViewState::Default;
}

- (void)performLimitedMigration:(BOOL)limited
{
  auto & f = GetFramework();
  LocationManager * lm = [MapsAppDelegate theApp].m_locationManager;
  ms::LatLon position{};
  if (![lm getLat:position.lat Lon:position.lon])
    position = MercatorBounds::ToLatLon(f.GetViewportCenter());

  auto migrate = ^
  {
    GetFramework().Migrate(!limited);
    [self performSegueWithIdentifier:kDownloaderSegue sender:self];
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
    static_cast<MWMMigrationView *>(self.view).state = MWMMigrationViewState::Processing;
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
      [avc presentInternalErrorAlert];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [avc presentDownloaderNotEnoughSpaceAlert];
      break;
    case NodeErrorCode::NoInetConnection:
      [avc presentDownloaderNoConnectionAlertWithOkBlock:^
      {
        [MWMStorage retryDownloadNode:countryId];
      }];
      break;
  }
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
