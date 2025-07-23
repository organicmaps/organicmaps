#import "MWMStorage.h"
#import "MWMMapNodeAttributes+Core.h"
#import "MWMMapUpdateInfo+Core.h"

#include <CoreApi/Framework.h>

#include "storage/country_info_getter.hpp"
#include "storage/storage_helpers.hpp"

NSErrorDomain const kStorageErrorDomain = @"com.mapswithme.storage";

NSInteger const kStorageNotEnoughSpace = 1;
NSInteger const kStorageNoConnection = 2;
NSInteger const kStorageCellularForbidden = 3;
NSInteger const kStorageRoutingActive = 4;
NSInteger const kStorageHaveUnsavedEdits = 5;

using namespace storage;

@interface MWMStorage ()

@property(nonatomic, strong) NSHashTable<id<MWMStorageObserver>> * observers;

@end

@implementation MWMStorage

+ (instancetype)sharedStorage
{
  static MWMStorage * instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ instance = [[self alloc] init]; });
  return instance;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _observers = [NSHashTable weakObjectsHashTable];
    NSHashTable * observers = _observers;

    GetFramework().GetStorage().Subscribe(
        [observers](CountryId const & countryId)
    {
      // A copy is created, because MWMMapDownloadDialog is unsubscribed inside this notification with
      // NSGenericException', reason: '*** Collection <NSConcreteHashTable> was mutated while being enumerated.'
      NSHashTable * observersCopy = [observers copy];
      for (id<MWMStorageObserver> observer in observersCopy)
        [observer processCountryEvent:@(countryId.c_str())];
    }, [observers](CountryId const & countryId, downloader::Progress const & progress)
    {
      for (id<MWMStorageObserver> observer in observers)
      {
        // processCountry function in observer's implementation may not exist.
        /// @todo We can face with an invisible bug, if function's signature will be changed.
        if ([observer respondsToSelector:@selector(processCountry:downloadedBytes:totalBytes:)])
        {
          [observer processCountry:@(countryId.c_str())
                   downloadedBytes:progress.m_bytesDownloaded
                        totalBytes:progress.m_bytesTotal];
        }
      }
    });
  }
  return self;
}

- (void)addObserver:(id<MWMStorageObserver>)observer
{
  [self.observers addObject:observer];
}

- (void)removeObserver:(id<MWMStorageObserver>)observer
{
  [self.observers removeObject:observer];
}

- (BOOL)downloadNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error
{
  if (IsEnoughSpaceForDownload(countryId.UTF8String, GetFramework().GetStorage()))
  {
    NSError * connectionError;
    if ([self checkConnection:&connectionError])
    {
      GetFramework().GetStorage().DownloadNode(countryId.UTF8String);
      return YES;
    }
    else if (error)
    {
      *error = connectionError;
    }
  }
  else if (error)
  {
    *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageNotEnoughSpace userInfo:nil];
  }

  return NO;
}

- (void)retryDownloadNode:(NSString *)countryId
{
  if ([self checkConnection:nil])
    GetFramework().GetStorage().RetryDownloadNode(countryId.UTF8String);
}

- (BOOL)updateNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error
{
  if (IsEnoughSpaceForUpdate(countryId.UTF8String, GetFramework().GetStorage()))
  {
    NSError * connectionError;
    if ([self checkConnection:&connectionError])
    {
      GetFramework().GetStorage().UpdateNode(countryId.UTF8String);
      return YES;
    }
    else if (error)
    {
      *error = connectionError;
    }
  }
  else if (error)
  {
    *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageNotEnoughSpace userInfo:nil];
  }

  return NO;
}

- (BOOL)deleteNode:(NSString *)countryId
    ignoreUnsavedEdits:(BOOL)force
                 error:(NSError * __autoreleasing _Nullable *)error
{
  auto & f = GetFramework();
  if (f.GetRoutingManager().IsRoutingActive())
  {
    if (error)
      *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageRoutingActive userInfo:nil];
    return NO;
  }

  if (!force && f.HasUnsavedEdits(countryId.UTF8String))
  {
    if (error)
      *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageHaveUnsavedEdits userInfo:nil];
  }
  else
  {
    f.GetStorage().DeleteNode(countryId.UTF8String);
    return YES;
  }

  return NO;
}

- (void)cancelDownloadNode:(NSString *)countryId
{
  GetFramework().GetStorage().CancelDownloadNode(countryId.UTF8String);
}

- (void)showNode:(NSString *)countryId
{
  GetFramework().ShowNode(countryId.UTF8String);
}

- (BOOL)downloadNodes:(NSArray<NSString *> *)countryIds error:(NSError * __autoreleasing _Nullable *)error
{
  auto & s = GetFramework().GetStorage();

  MwmSize requiredSize = 0;
  for (NSString * countryId in countryIds)
  {
    NodeAttrs nodeAttrs;
    GetFramework().GetStorage().GetNodeAttrs(countryId.UTF8String, nodeAttrs);
    requiredSize += nodeAttrs.m_mwmSize;
  }

  if (storage::IsEnoughSpaceForDownload(requiredSize))
  {
    NSError * connectionError;
    if ([self checkConnection:&connectionError])
    {
      for (NSString * countryId in countryIds)
        s.DownloadNode(countryId.UTF8String);
      return YES;
    }
    else if (error)
    {
      *error = connectionError;
    }
  }
  else if (error)
  {
    *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageNotEnoughSpace userInfo:nil];
  }

  return NO;
}

- (BOOL)checkConnection:(NSError * __autoreleasing *)error
{
  switch (Platform::ConnectionStatus())
  {
  case Platform::EConnectionType::CONNECTION_NONE:
    if (error)
      *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageNoConnection userInfo:nil];
    return NO;
    break;
  case Platform::EConnectionType::CONNECTION_WIFI:
    if (error)
      *error = nil;
    return YES;
    break;
  case Platform::EConnectionType::CONNECTION_WWAN:
  {
    if (!GetFramework().GetDownloadingPolicy().IsCellularDownloadEnabled())
    {
      if (error)
        *error = [NSError errorWithDomain:kStorageErrorDomain code:kStorageCellularForbidden userInfo:nil];
      return NO;
    }
    else
    {
      if (error)
        *error = nil;
      return YES;
    }
    break;
  }
  }
}

- (BOOL)haveDownloadedCountries
{
  return GetFramework().GetStorage().HaveDownloadedCountries();
}

- (BOOL)downloadInProgress
{
  return GetFramework().GetStorage().IsDownloadInProgress();
}

- (void)enableCellularDownload:(BOOL)enable
{
  GetFramework().GetDownloadingPolicy().EnableCellularDownload(enable);
}

#pragma mark - Attributes

- (NSArray<NSString *> *)allCountries
{
  NSString * rootId = @(GetFramework().GetStorage().GetRootId().c_str());
  return [self allCountriesWithParent:rootId];
}

- (NSArray<NSString *> *)allCountriesWithParent:(NSString *)countryId
{
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String, downloadedChildren, availableChildren,
                                                  true /* keepAvailableChildren */);

  NSMutableArray * result = [NSMutableArray arrayWithCapacity:availableChildren.size()];
  for (auto const & cid : availableChildren)
    [result addObject:@(cid.c_str())];
  return [result copy];
}

- (NSArray<NSString *> *)availableCountriesWithParent:(NSString *)countryId
{
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String, downloadedChildren, availableChildren);

  NSMutableArray * result = [NSMutableArray arrayWithCapacity:availableChildren.size()];
  for (auto const & cid : availableChildren)
    [result addObject:@(cid.c_str())];
  return [result copy];
}

- (NSArray<NSString *> *)downloadedCountries
{
  NSString * rootId = @(GetFramework().GetStorage().GetRootId().c_str());
  return [self downloadedCountriesWithParent:rootId];
}

- (NSArray<NSString *> *)downloadedCountriesWithParent:(NSString *)countryId
{
  storage::CountriesVec downloadedChildren;
  storage::CountriesVec availableChildren;
  GetFramework().GetStorage().GetChildrenInGroups(countryId.UTF8String, downloadedChildren, availableChildren);

  NSMutableArray * result = [NSMutableArray arrayWithCapacity:downloadedChildren.size()];
  for (auto const & cid : downloadedChildren)
    [result addObject:@(cid.c_str())];
  return [result copy];
}

- (MWMMapNodeAttributes *)attributesForCountry:(NSString *)countryId
{
  auto const & s = GetFramework().GetStorage();
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

- (MWMMapNodeAttributes *)attributesForRoot
{
  return [self attributesForCountry:@(GetFramework().GetStorage().GetRootId().c_str())];
}

- (NSString *)getRootId
{
  return @(GetFramework().GetStorage().GetRootId().c_str());
}

- (NSString *)nameForCountry:(NSString *)countryId
{
  return @(GetFramework().GetStorage().GetNodeLocalName(countryId.UTF8String).c_str());
}

- (NSArray<NSString *> *)nearbyAvailableCountries:(CLLocationCoordinate2D)location
{
  auto & f = GetFramework();
  storage::CountriesVec closestCoutryIds;
  f.GetCountryInfoGetter().GetRegionsCountryId(mercator::FromLatLon(location.latitude, location.longitude),
                                               closestCoutryIds);
  NSMutableArray * nearmeCountries = [NSMutableArray array];
  for (auto const & countryId : closestCoutryIds)
  {
    storage::NodeStatuses nodeStatuses;
    f.GetStorage().GetNodeStatuses(countryId, nodeStatuses);
    if (nodeStatuses.m_status != storage::NodeStatus::OnDisk)
      [nearmeCountries addObject:@(countryId.c_str())];
  }

  return nearmeCountries.count > 0 ? [nearmeCountries copy] : nil;
}

- (MWMMapUpdateInfo *)updateInfoWithParent:(nullable NSString *)countryId
{
  auto const & s = GetFramework().GetStorage();
  Storage::UpdateInfo updateInfo;
  if (countryId.length > 0)
    s.GetUpdateInfo(countryId.UTF8String, updateInfo);
  else
    s.GetUpdateInfo(s.GetRootId(), updateInfo);
  return [[MWMMapUpdateInfo alloc] initWithUpdateInfo:updateInfo];
}

@end
