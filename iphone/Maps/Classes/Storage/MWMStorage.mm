#import "Common.h"
#import "MWMStorage.h"

#include "Framework.h"

#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

using namespace storage;

@implementation MWMStorage

+ (void)downloadNode:(TCountryId const &)countryId
     alertController:(MWMAlertViewController *)alertController
           onSuccess:(TMWMVoidBlock)onSuccess
{
  if (IsEnoughSpaceForDownload(countryId, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId, onSuccess]
    {
      GetFramework().GetStorage().DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    } alertController:alertController];
  }
  else
  {
    [alertController presentNotEnoughSpaceAlert];
  }
}

+ (void)retryDownloadNode:(TCountryId const &)countryId
{
  GetFramework().GetStorage().RetryDownloadNode(countryId);
}

+ (void)updateNode:(TCountryId const &)countryId
   alertController:(MWMAlertViewController *)alertController
{
  if (IsEnoughSpaceForUpdate(countryId, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId]
    {
      GetFramework().GetStorage().UpdateNode(countryId);
    } alertController:alertController];
  }
  else
  {
    [alertController presentNotEnoughSpaceAlert];
  }
}

+ (void)deleteNode:(TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  auto & f = GetFramework();
  if (f.IsRoutingActive())
  {
    [alertController presentDeleteMapProhibitedAlert];
    return;
  }

  if (f.HasUnsavedEdits(countryId))
  {
    [alertController presentUnsavedEditsAlertWithOkBlock:[countryId]
    {
      GetFramework().GetStorage().DeleteNode(countryId);
    }];
  }
  else
  {
    f.GetStorage().DeleteNode(countryId);
  }
}

+ (void)cancelDownloadNode:(TCountryId const &)countryId
{
  GetFramework().GetStorage().CancelDownloadNode(countryId);
}

+ (void)showNode:(TCountryId const &)countryId
{
  GetFramework().ShowNode(countryId);
}

+ (void)downloadNodes:(TCountriesVec const &)countryIds
      alertController:(MWMAlertViewController *)alertController
            onSuccess:(TMWMVoidBlock)onSuccess
{
  TMwmSize requiredSize = accumulate(countryIds.begin(), countryIds.end(), kMaxMwmSizeBytes,
                                     [](size_t const & size, TCountryId const & countryId)
                                     {
                                       NodeAttrs nodeAttrs;
                                       GetFramework().GetStorage().GetNodeAttrs(countryId, nodeAttrs);
                                       return size + nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize;
                                     });
  if (GetPlatform().GetWritableStorageStatus(requiredSize) == Platform::TStorageStatus::STORAGE_OK)
  {
    [self checkConnectionAndPerformAction:[countryIds, onSuccess]
    {
      auto & s = GetFramework().GetStorage();
      for (auto const & countryId : countryIds)
        s.DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    } alertController:alertController];
  }
  else
  {
    [alertController presentNotEnoughSpaceAlert];
  }
}

+ (void)checkConnectionAndPerformAction:(TMWMVoidBlock)action
                        alertController:(MWMAlertViewController *)alertController
{
  switch (Platform::ConnectionStatus())
  {
    case Platform::EConnectionType::CONNECTION_NONE:
      [alertController presentNoConnectionAlert];
      break;
    case Platform::EConnectionType::CONNECTION_WIFI:
      action();
      break;
    case Platform::EConnectionType::CONNECTION_WWAN:
    {
      if (!GetFramework().GetDownloadingPolicy().IsCellularDownloadEnabled())
      {
        [alertController presentNoWiFiAlertWithOkBlock:[action]
        {
          GetFramework().GetDownloadingPolicy().EnableCellularDownload(true);
          action();
        }];
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
