#import "MWMStorage.h"
#import "MWMAlertViewController.h"

#include <CoreApi/Framework.h>
#import <CoreApi/MWMMapNodeAttributes+Core.h>
#import <CoreApi/MWMMapUpdateInfo+Core.h>

#include "storage/storage_helpers.hpp"

using namespace storage;

@implementation MWMStorage

+ (void)downloadNode:(NSString *)countryId onSuccess:(MWMVoidBlock)onSuccess onCancel:(MWMVoidBlock)onCancel
{
  if (IsEnoughSpaceForDownload(countryId.UTF8String, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId, onSuccess] {
      GetFramework().GetStorage().DownloadNode(countryId.UTF8String);
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

+ (void)retryDownloadNode:(NSString *)countryId
{
  [self checkConnectionAndPerformAction:[countryId] {
    GetFramework().GetStorage().RetryDownloadNode(countryId.UTF8String);
  } cancelAction:nil];
}

+ (void)updateNode:(NSString *)countryId onCancel:(MWMVoidBlock)onCancel
{
  if (IsEnoughSpaceForUpdate(countryId.UTF8String, GetFramework().GetStorage()))
  {
    [self checkConnectionAndPerformAction:[countryId] {
      GetFramework().GetStorage().UpdateNode(countryId.UTF8String);
    } cancelAction:onCancel];
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
    if (onCancel)
      onCancel();
  }
}

+ (void)deleteNode:(NSString *)countryId
{
  auto & f = GetFramework();
  if (f.GetRoutingManager().IsRoutingActive())
  {
    [[MWMAlertViewController activeAlertController] presentDeleteMapProhibitedAlert];
    return;
  }

  if (f.HasUnsavedEdits(countryId.UTF8String))
  {
    [[MWMAlertViewController activeAlertController]
        presentUnsavedEditsAlertWithOkBlock:[countryId] {
          GetFramework().GetStorage().DeleteNode(countryId.UTF8String);
        }];
  }
  else
  {
    f.GetStorage().DeleteNode(countryId.UTF8String);
  }
}

+ (void)cancelDownloadNode:(NSString *)countryId
{
  GetFramework().GetStorage().CancelDownloadNode(countryId.UTF8String);
}

+ (void)showNode:(NSString *)countryId { GetFramework().ShowNode(countryId.UTF8String); }
+ (void)downloadNodes:(NSArray<NSString *> *)countryIds onSuccess:(MWMVoidBlock)onSuccess onCancel:(MWMVoidBlock)onCancel
{
  auto & s = GetFramework().GetStorage();

  MwmSize requiredSize = s.GetMaxMwmSizeBytes();
  for (NSString *countryId in countryIds) {
    NodeAttrs nodeAttrs;
    GetFramework().GetStorage().GetNodeAttrs(countryId.UTF8String, nodeAttrs);
    requiredSize += nodeAttrs.m_mwmSize;
  }
  if (storage::IsEnoughSpaceForDownload(requiredSize))
  {
    [self checkConnectionAndPerformAction:[countryIds, onSuccess, &s] {
      for (NSString *countryId in countryIds)
        s.DownloadNode(countryId.UTF8String);
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

+ (BOOL)haveDownloadedCountries {
  return GetFramework().GetStorage().HaveDownloadedCountries();
}

+ (BOOL)downloadInProgress {
  return GetFramework().GetStorage().IsDownloadInProgress();
}

#pragma mark - Attributes

+ (NSArray<NSString *> *)allCountries {
  NSString *rootId = @(GetFramework().GetStorage().GetRootId().c_str());
  return [self allCountriesWithParent:rootId];
}

+ (NSArray<NSString *> *)allCountriesWithParent:(NSString *)countryId {
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String,
                                                  downloadedChildren,
                                                  availableChildren,
                                                  true /* keepAvailableChildren */);

  NSMutableArray *result = [NSMutableArray arrayWithCapacity:availableChildren.size()];
  for (auto const &cid : availableChildren) {
    [result addObject:@(cid.c_str())];
  }
  return [result copy];
}

+ (NSArray<NSString *> *)availableCountriesWithParent:(NSString *)countryId {
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String,
                                                  downloadedChildren,
                                                  availableChildren);

  NSMutableArray *result = [NSMutableArray arrayWithCapacity:availableChildren.size()];
  for (auto const &cid : availableChildren) {
    [result addObject:@(cid.c_str())];
  }
  return [result copy];
}

+ (NSArray<NSString *> *)downloadedCountries {
  NSString *rootId = @(GetFramework().GetStorage().GetRootId().c_str());
  return [self downloadedCountriesWithParent:rootId];
}

+ (NSArray<NSString *> *)downloadedCountriesWithParent:(NSString *)countryId {
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String,
                                                  downloadedChildren,
                                                  availableChildren);

  NSMutableArray *result = [NSMutableArray arrayWithCapacity:downloadedChildren.size()];
  for (auto const &cid : downloadedChildren) {
    [result addObject:@(cid.c_str())];
  }
  return [result copy];
}

+ (MWMMapNodeAttributes *)attributesForCountry:(NSString *)countryId {
  auto const &s = GetFramework().GetStorage();
  storage::NodeAttrs nodeAttrs;
  s.GetNodeAttrs(countryId.UTF8String, nodeAttrs);
  storage::CountriesVec children;
  s.GetChildren(countryId.UTF8String, children);
  BOOL isParentRoot = nodeAttrs.m_parentInfo.size() == 1 && nodeAttrs.m_parentInfo[0].m_id == s.GetRootId();
  return [[MWMMapNodeAttributes alloc] initWithCoreAttributes:nodeAttrs
                                                    countryId:countryId
                                                    hasParent:!isParentRoot
                                                  hasChildren:!children.empty()];
}

+ (MWMMapNodeAttributes *)attributesForRoot {
  return [self attributesForCountry:@(GetFramework().GetStorage().GetRootId().c_str())];
}

+ (NSString *)nameForCountry:(NSString *)countryId {
  return @(GetFramework().GetStorage().GetNodeLocalName(countryId.UTF8String).c_str());
}

+ (NSArray<NSString *> *)nearbyAvailableCountries:(CLLocationCoordinate2D)location {
  auto &f = GetFramework();
  storage::CountriesVec closestCoutryIds;
  f.GetCountryInfoGetter().GetRegionsCountryId(mercator::FromLatLon(location.latitude, location.longitude),
                                               closestCoutryIds);
  NSMutableArray *nearmeCountries = [NSMutableArray array];
  for (auto const &countryId : closestCoutryIds) {
    storage::NodeStatuses nodeStatuses;
    f.GetStorage().GetNodeStatuses(countryId, nodeStatuses);
    if (nodeStatuses.m_status != storage::NodeStatus::OnDisk)
      [nearmeCountries addObject:@(countryId.c_str())];
  }

  return nearmeCountries.count > 0 ? [nearmeCountries copy] : nil;
}

+ (MWMMapUpdateInfo *)updateInfoWithParent:(nullable NSString *)countryId {
  auto const &s = GetFramework().GetStorage();
  Storage::UpdateInfo updateInfo;
  if (countryId.length > 0) {
    s.GetUpdateInfo(countryId.UTF8String, updateInfo);
  } else {
    s.GetUpdateInfo(s.GetRootId(), updateInfo);
  }
  return [[MWMMapUpdateInfo alloc] initWithUpdateInfo:updateInfo];
}

@end
