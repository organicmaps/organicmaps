#import "Common.h"
#import "MWMStorage.h"

#include "Framework.h"

#include "platform/platform.hpp"

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
  [self performAction:^
  {
    GetFramework().Storage().DownloadNode(countryId);
    if (onSuccess)
      onSuccess();
  }
  alertController:alertController];
}

+ (void)retryDownloadNode:(TCountryId const &)countryId
{
  GetFramework().Storage().RetryDownloadNode(countryId);
}

+ (void)updateNode:(TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  [self performAction:^
  {
    GetFramework().Storage().UpdateNode(countryId);
  }
  alertController:alertController];
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

+ (void)downloadNodes:(storage::TCountriesVec const &)countryIds alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess
{
  [self performAction:[countryIds, onSuccess]
  {
    auto & s = GetFramework().Storage();
    for (auto const & countryId : countryIds)
      s.DownloadNode(countryId);
    if (onSuccess)
      onSuccess();
  }
  alertController:alertController];
}

+ (void)performAction:(TMWMVoidBlock)action alertController:(MWMAlertViewController *)alertController
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
