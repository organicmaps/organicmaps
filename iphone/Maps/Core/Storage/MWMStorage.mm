#import "MWMStorage.h"
#import "MWMAlertViewController.h"
#import "MWMRouter.h"

#include <CoreApi/Framework.h>

#include "storage/storage_helpers.hpp"

using namespace storage;

@implementation MWMStorage

+ (void)downloadNode:(CountryId const &)countryId onSuccess:(MWMVoidBlock)onSuccess onCancel:(MWMVoidBlock)onCancel
{
  if (IsEnoughSpaceForDownload(countryId, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId, onSuccess] {
      GetFramework().GetStorage().DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    } cancelAction:onCancel];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
    if (onCancel)
      onCancel();
  }
}

+ (void)retryDownloadNode:(CountryId const &)countryId
{
  [self checkConnectionAndPerformAction:[countryId] {
    GetFramework().GetStorage().RetryDownloadNode(countryId);
  } cancelAction:nil];
}

+ (void)updateNode:(CountryId const &)countryId onCancel:(MWMVoidBlock)onCancel
{
  if (IsEnoughSpaceForUpdate(countryId, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId] {
      GetFramework().GetStorage().UpdateNode(countryId);
    } cancelAction:onCancel];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
    if (onCancel)
      onCancel();
  }
}

+ (void)deleteNode:(CountryId const &)countryId
{
  if ([MWMRouter isRoutingActive])
  {
    [[MWMAlertViewController activeAlertController] presentDeleteMapProhibitedAlert];
    return;
  }

  auto & f = GetFramework();
  if (f.HasUnsavedEdits(countryId))
  {
    [[MWMAlertViewController activeAlertController]
        presentUnsavedEditsAlertWithOkBlock:[countryId] {
          GetFramework().GetStorage().DeleteNode(countryId);
        }];
  }
  else
  {
    f.GetStorage().DeleteNode(countryId);
  }
}

+ (void)cancelDownloadNode:(CountryId const &)countryId
{
  GetFramework().GetStorage().CancelDownloadNode(countryId);
}

+ (void)showNode:(CountryId const &)countryId { GetFramework().ShowNode(countryId); }
+ (void)downloadNodes:(CountriesVec const &)countryIds onSuccess:(MWMVoidBlock)onSuccess onCancel:(MWMVoidBlock)onCancel
{
  auto & s = GetFramework().GetStorage();
  MwmSize requiredSize =
      std::accumulate(countryIds.begin(), countryIds.end(), s.GetMaxMwmSizeBytes(),
                      [](size_t const & size, CountryId const & countryId) {
                        NodeAttrs nodeAttrs;
                        GetFramework().GetStorage().GetNodeAttrs(countryId, nodeAttrs);
                        return size + nodeAttrs.m_mwmSize;
                      });
  if (storage::IsEnoughSpaceForDownload(requiredSize))
  {
    [self checkConnectionAndPerformAction:[countryIds, onSuccess, &s] {
      for (auto const & countryId : countryIds)
        s.DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    } cancelAction: onCancel];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
    if (onCancel)
      onCancel();
  }
}

+ (void)checkConnectionAndPerformAction:(MWMVoidBlock)action cancelAction:(MWMVoidBlock)cancel
{
  switch (Platform::ConnectionStatus())
  {
    case Platform::EConnectionType::CONNECTION_NONE:
      [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
      if (cancel)
        cancel();
      break;
    case Platform::EConnectionType::CONNECTION_WIFI:
      action();
      break;
    case Platform::EConnectionType::CONNECTION_WWAN:
    {
      if (!GetFramework().GetDownloadingPolicy().IsCellularDownloadEnabled())
      {
        [[MWMAlertViewController activeAlertController] presentNoWiFiAlertWithOkBlock:[action] {
          GetFramework().GetDownloadingPolicy().EnableCellularDownload(true);
          action();
        } andCancelBlock:cancel];
      }
      else
      {
        action();
      }
      break;
    }
  }
}

@end
