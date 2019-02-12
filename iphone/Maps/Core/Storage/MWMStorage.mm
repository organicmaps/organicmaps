#import "MWMStorage.h"
#import "MWMAlertViewController.h"
#import "MWMRouter.h"
#import "MWMFrameworkHelper.h"

#include "Framework.h"

#include "storage/storage_helpers.hpp"

#include <numeric>

using namespace storage;

@implementation MWMStorage

+ (void)downloadNode:(CountryId const &)countryId onSuccess:(MWMVoidBlock)onSuccess
{
  if (IsEnoughSpaceForDownload(countryId, GetFramework().GetStorage()))
  {
    [MWMFrameworkHelper checkConnectionAndPerformAction:[countryId, onSuccess] {
      GetFramework().GetStorage().DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    }];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
  }
}

+ (void)retryDownloadNode:(CountryId const &)countryId
{
  [MWMFrameworkHelper checkConnectionAndPerformAction:[countryId] {
    GetFramework().GetStorage().RetryDownloadNode(countryId);
  }];
}

+ (void)updateNode:(CountryId const &)countryId
{
  if (IsEnoughSpaceForUpdate(countryId, GetFramework().GetStorage()))
  {
    [MWMFrameworkHelper checkConnectionAndPerformAction:[countryId] {
      GetFramework().GetStorage().UpdateNode(countryId);
    }];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
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
+ (void)downloadNodes:(CountriesVec const &)countryIds onSuccess:(MWMVoidBlock)onSuccess
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
    [MWMFrameworkHelper checkConnectionAndPerformAction:[countryIds, onSuccess, &s] {
      for (auto const & countryId : countryIds)
        s.DownloadNode(countryId);
      if (onSuccess)
        onSuccess();
    }];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
  }
}

@end
