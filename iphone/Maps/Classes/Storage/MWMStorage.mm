#import "Common.h"
#import "MWMStorage.h"

#include "Framework.h"

#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

namespace
{
NSString * const kStorageCanShowNoWifiAlert = @"StorageCanShowNoWifiAlert";
} // namespace

using namespace storage;

@implementation MWMStorage

+ (void)startSession
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:YES forKey:kStorageCanShowNoWifiAlert];
  [ud synchronize];
}

+ (void)downloadNode:(TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess
{
  [self checkEnoughSpaceFor:countryId andPerformAction:^
  {
    GetFramework().Storage().DownloadNode(countryId);
    if (onSuccess)
      onSuccess();
  } alertController:alertController];
}

+ (void)retryDownloadNode:(TCountryId const &)countryId
{
  GetFramework().Storage().RetryDownloadNode(countryId);
}

+ (void)updateNode:(TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  [self checkEnoughSpaceFor:countryId andPerformAction:^
  {
    GetFramework().Storage().UpdateNode(countryId);
  } alertController:alertController];
}

+ (void)deleteNode:(TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  if (GetFramework().HasUnsavedEdits(countryId))
  {
    [alertController presentUnsavedEditsAlertWithOkBlock:[countryId]
    {
      GetFramework().Storage().DeleteNode(countryId);
    }];
  }
  else
  {
    GetFramework().Storage().DeleteNode(countryId);
  }
}

+ (void)cancelDownloadNode:(TCountryId const &)countryId
{
  GetFramework().Storage().CancelDownloadNode(countryId);
}

+ (void)showNode:(TCountryId const &)countryId
{
  GetFramework().ShowNode(countryId);
}

+ (void)downloadNodes:(TCountriesVec const &)countryIds
      alertController:(MWMAlertViewController *)alertController
            onSuccess:(TMWMVoidBlock)onSuccess
{
  size_t requiredSize = accumulate(countryIds.begin(), countryIds.end(), 0,
                                   [](size_t const & size, TCountryId const & countryId)
                                   {
                                     NodeAttrs nodeAttrs;
                                     GetFramework().Storage().GetNodeAttrs(countryId, nodeAttrs);
                                     return size + nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize;
                                   });
  size_t constexpr kDownloadExtraSpaceSize = 100 * MB;
  requiredSize += kDownloadExtraSpaceSize;
  if (GetPlatform().GetWritableStorageStatus(requiredSize) == Platform::TStorageStatus::STORAGE_OK)
  {
    [self checkConnectionAndPerformAction:[countryIds, onSuccess]
    {
      auto & s = GetFramework().Storage();
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

+ (void)checkEnoughSpaceFor:(TCountryId const &)countryId
           andPerformAction:(TMWMVoidBlock)action
            alertController:(MWMAlertViewController *)alertController
{
  if (IsEnoughSpaceForDownload(countryId, GetFramework().Storage()))
    [self checkConnectionAndPerformAction:action alertController:alertController];
  else
    [alertController presentNotEnoughSpaceAlert];
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
      NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
      if ([ud boolForKey:kStorageCanShowNoWifiAlert])
      {
        [alertController presentNoWiFiAlertWithOkBlock:^
        {
          [ud setBool:NO forKey:kStorageCanShowNoWifiAlert];
          [ud synchronize];
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
