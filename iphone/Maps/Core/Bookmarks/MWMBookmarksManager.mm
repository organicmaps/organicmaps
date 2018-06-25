#import "MWMBookmarksManager.h"
#import "MWMCatalogCategory+Convenience.h"
#import "MWMCatalogObserver.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "coding/internal/file_data.hpp"

#include "base/stl_helpers.hpp"

#include <utility>

namespace
{
using Observer = id<MWMBookmarksObserver>;
using Observers = NSHashTable<Observer>;

using TLoopBlock = void (^)(Observer observer);
  
NSString * const CloudErrorToString(Cloud::SynchronizationResult result)
{
  switch (result)
  {
  case Cloud::SynchronizationResult::Success: return nil;
  case Cloud::SynchronizationResult::AuthError: return kStatAuth;
  case Cloud::SynchronizationResult::NetworkError: return kStatNetwork;
  case Cloud::SynchronizationResult::DiskError: return kStatDisk;
  case Cloud::SynchronizationResult::UserInterrupted: return kStatUserInterrupted;
  case Cloud::SynchronizationResult::InvalidCall: return kStatInvalidCall;
  }
}
}  // namespace

@interface MWMBookmarksManager ()

@property(nonatomic) Observers * observers;
@property(nonatomic) BOOL areBookmarksLoaded;
@property(nonatomic) NSURL * shareCategoryURL;

@property(nonatomic) NSMutableDictionary<NSString *, MWMCatalogObserver*> * catalogObservers;

@end

@implementation MWMBookmarksManager

+ (instancetype)manager
{
  static MWMBookmarksManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

+ (void)addObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[MWMBookmarksManager manager].observers addObject:observer];
  });
}

+ (void)removeObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[MWMBookmarksManager manager].observers removeObject:observer];
  });
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];
    [self registerBookmarksObserver];
  }
  return self;
}

- (void)registerBookmarksObserver
{
  BookmarkManager::AsyncLoadingCallbacks bookmarkCallbacks;
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onStarted = [wSelf]() {
      wSelf.areBookmarksLoaded = NO;
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFinished = [wSelf]() {
      __strong auto self = wSelf;
      if (!self)
        return;
      self.areBookmarksLoaded = YES;
      [self loopObservers:^(Observer observer) {
        if ([observer respondsToSelector:@selector(onBookmarksLoadFinished)])
          [observer onBookmarksLoadFinished];
      }];
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFileSuccess = [wSelf](std::string const & filePath,
                                                bool isTemporaryFile) {
      __strong auto self = wSelf;
      if (!self)
        return;
      [self processFileEvent:YES];
      [self loopObservers:^(Observer observer) {
        if ([observer respondsToSelector:@selector(onBookmarksFileLoadSuccess)])
          [observer onBookmarksFileLoadSuccess];
      }];
      [Statistics logEvent:kStatEventName(kStatApplication, kStatImport)
            withParameters:@{kStatValue: kStatKML}];
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFileError = [wSelf](std::string const & filePath, bool isTemporaryFile) {
      [wSelf processFileEvent:NO];
    };
  }
  GetFramework().GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(bookmarkCallbacks));
}

+ (BOOL)areBookmarksLoaded { return [MWMBookmarksManager manager].areBookmarksLoaded; }

+ (void)loadBookmarks
{
  [MWMBookmarksManager manager];
  
  auto & bm = GetFramework().GetBookmarkManager();
  
  auto onSynchronizationStarted = [](Cloud::SynchronizationType type)
  {
    if (type == Cloud::SynchronizationType::Backup)
    {
      [Statistics logEvent:kStatBookmarksSyncStarted];
    }
    else
    {
      [[MWMBookmarksManager manager] loopObservers:^(Observer observer) {
        if ([observer respondsToSelector:@selector(onRestoringStarted)])
          [observer onRestoringStarted];
      }];
    }
  };
  
  auto onSynchronizationFinished = [](Cloud::SynchronizationType type, Cloud::SynchronizationResult result,
                                      std::string const & errorStr)
  {
    if (result == Cloud::SynchronizationResult::Success)
    {
      [Statistics logEvent:type == Cloud::SynchronizationType::Backup ? kStatBookmarksSyncSuccess :
                                                                        kStatBookmarksRestoreProposalSuccess];
    }
    else if (auto const error = CloudErrorToString(result))
    {
      [Statistics logEvent:type == Cloud::SynchronizationType::Backup ? kStatBookmarksSyncError :
                                                                        kStatBookmarksRestoreProposalError
            withParameters:@{kStatType: error, kStatError: @(errorStr.c_str())}];
    }

    [[MWMBookmarksManager manager] loopObservers:^(Observer observer) {
      if ([observer respondsToSelector:@selector(onSynchronizationFinished:)])
        [observer onSynchronizationFinished:static_cast<MWMSynchronizationResult>(my::Key(result))];
    }];
  };
  
  auto onRestoreRequested = [](Cloud::RestoringRequestResult result, std::string const & deviceName,
                               uint64_t backupTimestampInMs)
  {
    auto const res = static_cast<MWMRestoringRequestResult>(my::Key(result));
    NSDate * date = nil;

    if (result == Cloud::RestoringRequestResult::BackupExists)
    {
        auto const interval = static_cast<NSTimeInterval>(backupTimestampInMs / 1000.);
        date = [NSDate dateWithTimeIntervalSince1970:interval];
    }
    else if (result == Cloud::RestoringRequestResult::NoBackup)
    {
      [Statistics logEvent:kStatBookmarksRestoreProposalError
            withParameters:@{kStatType: kStatNoBackup, kStatError: @("")}];
    }
    else if (result == Cloud::RestoringRequestResult::NotEnoughDiskSpace)
    {
      [Statistics logEvent:kStatBookmarksRestoreProposalError
            withParameters:@{kStatType: kStatDisk, kStatError: @("Not enough disk space")}];
    }

    [[MWMBookmarksManager manager] loopObservers:^(Observer observer) {
      if ([observer respondsToSelector:@selector(onRestoringRequest:deviceName:backupDate:)])
        [observer onRestoringRequest:res deviceName:@(deviceName.c_str()) backupDate:date];
    }];
  };
  
  auto onRestoredFilesPrepared = []
  {
    [[MWMBookmarksManager manager] loopObservers:^(Observer observer) {
      if ([observer respondsToSelector:@selector(onRestoringFilesPrepared)])
        [observer onRestoringFilesPrepared];
    }];
  };
  
  bm.SetCloudHandlers(std::move(onSynchronizationStarted), std::move(onSynchronizationFinished),
                      std::move(onRestoreRequested), std::move(onRestoredFilesPrepared));
  bm.LoadBookmarks();
}

+ (MWMGroupIDCollection)groupsIdList
{
  auto const & list = GetFramework().GetBookmarkManager().GetBmGroupsIdList();
  NSMutableArray<NSNumber *> * collection = @[].mutableCopy;
  for (auto const & groupId : list)
  {
    if (!GetFramework().GetBookmarkManager().IsCategoryFromCatalog(groupId))
      [collection addObject:@(groupId)];
  }
  return collection.copy;
}

+ (NSString *)getCategoryName:(MWMMarkGroupID)groupId
{
  return @(GetFramework().GetBookmarkManager().GetCategoryName(groupId).c_str());
}

+ (uint64_t)getCategoryMarksCount:(MWMMarkGroupID)groupId
{
  return GetFramework().GetBookmarkManager().GetUserMarkIds(groupId).size();
}

+ (uint64_t)getCategoryTracksCount:(MWMMarkGroupID)groupId
{
  return GetFramework().GetBookmarkManager().GetTrackIds(groupId).size();
}

+ (MWMMarkGroupID)createCategoryWithName:(NSString *)name
{
  auto groupId = GetFramework().GetBookmarkManager().CreateBookmarkCategory(name.UTF8String);
  GetFramework().GetBookmarkManager().SetLastEditedBmCategory(groupId);
  return groupId;
}

+ (void)setCategory:(MWMMarkGroupID)groupId name:(NSString *)name
{
  GetFramework().GetBookmarkManager().GetEditSession().SetCategoryName(groupId, name.UTF8String);
}

+ (BOOL)isCategoryVisible:(MWMMarkGroupID)groupId
{
  return GetFramework().GetBookmarkManager().IsVisible(groupId);
}

+ (void)setCategory:(MWMMarkGroupID)groupId isVisible:(BOOL)isVisible
{
  GetFramework().GetBookmarkManager().GetEditSession().SetIsVisible(groupId, isVisible);
}

+ (void)setUserCategoriesVisible:(BOOL)isVisible {
  GetFramework().GetBookmarkManager()
  .SetAllCategoriesVisibility(BookmarkManager::CategoryFilterType::Private, isVisible);
}

+ (void)setCatalogCategoriesVisible:(BOOL)isVisible {
  GetFramework().GetBookmarkManager()
  .SetAllCategoriesVisibility(BookmarkManager::CategoryFilterType::Public, isVisible);
}

+ (void)deleteCategory:(MWMMarkGroupID)groupId
{
  GetFramework().GetBookmarkManager().GetEditSession().DeleteBmCategory(groupId);
  [[self manager] loopObservers:^(Observer observer) {
    if ([observer respondsToSelector:@selector(onBookmarksCategoryDeleted:)])
      [observer onBookmarksCategoryDeleted:groupId];
  }];
}

+ (void)deleteBookmark:(MWMMarkID)bookmarkId
{
  GetFramework().GetBookmarkManager().GetEditSession().DeleteBookmark(bookmarkId);
  [[self manager] loopObservers:^(Observer observer) {
    if ([observer respondsToSelector:@selector(onBookmarkDeleted:)])
      [observer onBookmarkDeleted:bookmarkId];
  }];
}

+ (BOOL)checkCategoryName:(NSString *)name
{
  return !GetFramework().GetBookmarkManager().IsUsedCategoryName(name.UTF8String);
}

+ (void)shareCategory:(MWMMarkGroupID)groupId
{
  GetFramework().GetBookmarkManager().PrepareFileForSharing(groupId, [](auto sharingResult)
  {
    MWMBookmarksShareStatus status;
    MWMBookmarksManager * manager = [MWMBookmarksManager manager];
    switch (sharingResult.m_code)
    {
    case BookmarkManager::SharingResult::Code::Success:
    {
      manager.shareCategoryURL = [NSURL fileURLWithPath:@(sharingResult.m_sharingPath.c_str())
                                            isDirectory:NO];
      ASSERT(manager.shareCategoryURL, ("Invalid share category url"));
      status = MWMBookmarksShareStatusSuccess;
      break;
    }
    case BookmarkManager::SharingResult::Code::EmptyCategory:
      status = MWMBookmarksShareStatusEmptyCategory;
      break;
    case BookmarkManager::SharingResult::Code::ArchiveError:
      status = MWMBookmarksShareStatusArchiveError;
      break;
    case BookmarkManager::SharingResult::Code::FileError:
      status = MWMBookmarksShareStatusFileError;
      break;
    }
    
    [manager loopObservers:^(Observer observer) {
      if ([observer respondsToSelector:@selector(onBookmarksCategoryFilePrepared:)])
        [observer onBookmarksCategoryFilePrepared:status];
    }];
  });
}

+ (NSUInteger)filesCountForConversion
{
  return GetFramework().GetBookmarkManager().GetKmlFilesCountForConversion();
}

+ (void)convertAll
{
  GetFramework().GetBookmarkManager().ConvertAllKmlFiles([](bool success) {
    [[MWMBookmarksManager manager] loopObservers:^(Observer observer) {
      [observer onConversionFinish:success];
    }];
  });
}

+ (NSURL *)shareCategoryURL
{
  MWMBookmarksManager * manager = [MWMBookmarksManager manager];
  NSAssert(manager.shareCategoryURL != nil, @"Invalid share category url");
  return manager.shareCategoryURL;
}

+ (void)finishShareCategory
{
  MWMBookmarksManager * manager = [MWMBookmarksManager manager];
  if (!manager.shareCategoryURL)
    return;
  
  my::DeleteFileX(manager.shareCategoryURL.path.UTF8String);
  manager.shareCategoryURL = nil;
}

+ (NSDate *)lastSynchronizationDate
{
  auto timestampInMs = GetFramework().GetBookmarkManager().GetLastSynchronizationTimestampInMs();
  if (timestampInMs == 0)
    return nil;
  return [NSDate dateWithTimeIntervalSince1970:timestampInMs / 1000];
}

+ (BOOL)isCloudEnabled { return GetFramework().GetBookmarkManager().IsCloudEnabled(); }

+ (void)setCloudEnabled:(BOOL)enabled
{
  GetFramework().GetBookmarkManager().SetCloudEnabled(enabled);
}

+ (void)requestRestoring
{
  auto const status = Platform::ConnectionStatus();
  auto statusStr = [](Platform::EConnectionType type) -> NSString * {
    switch (type)
    {
    case Platform::EConnectionType::CONNECTION_NONE:
      return kStatOffline;
    case Platform::EConnectionType::CONNECTION_WWAN:
      return kStatMobile;
    case Platform::EConnectionType::CONNECTION_WIFI:
      return kStatWifi;
    }
  } (status);

  [Statistics logEvent:kStatBookmarksRestoreProposalClick
        withParameters:@{kStatNetwork : statusStr}];

  if (status == Platform::EConnectionType::CONNECTION_NONE)
  {
    [self.manager loopObservers:^(Observer observer) {
      if ([observer respondsToSelector:@selector(onRestoringRequest:deviceName:backupDate:)])
        [observer onRestoringRequest:MWMRestoringRequestResultNoInternet deviceName:nil backupDate:nil];
    }];
    return;
  }

  GetFramework().GetBookmarkManager().RequestCloudRestoring();
}

+ (void)applyRestoring
{
  GetFramework().GetBookmarkManager().ApplyCloudRestoring();
}

+ (void)cancelRestoring
{
  [Statistics logEvent:kStatBookmarksRestoreProposalCancel];
  GetFramework().GetBookmarkManager().CancelCloudRestoring();
}

- (void)loopObservers:(TLoopBlock)block
{
  for (Observer observer in self.observers)
  {
    if (observer)
      block(observer);
  }
}

- (void)processFileEvent:(BOOL)success
{
  UIAlertController * alert = [UIAlertController
      alertControllerWithTitle:L(@"load_kmz_title")
                       message:success ? L(@"load_kmz_successful") : L(@"load_kmz_failed")
                preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction * action =
      [UIAlertAction actionWithTitle:L(@"ok") style:UIAlertActionStyleDefault handler:nil];
  [alert addAction:action];
  alert.preferredAction = action;
  [[UIViewController topViewController] presentViewController:alert animated:YES completion:nil];
}

+ (void)setNotificationsEnabled:(BOOL)enabled
{
  GetFramework().GetBookmarkManager().SetNotificationsEnabled(enabled);
}

+ (BOOL)areNotificationsEnabled
{
  return GetFramework().GetBookmarkManager().AreNotificationsEnabled();
}

+ (NSURL * _Nullable )catalogFrontendUrl
{
  NSString *urlString = @(GetFramework().GetBookmarkManager().GetCatalog().GetFrontendUrl().c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

+ (NSURL * _Nullable)sharingUrlForCategoryId:(MWMMarkGroupID)groupId
{
  NSString *urlString = @(GetFramework().GetBookmarkManager().GetCategoryCatalogDeeplink(groupId).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

+ (void)downloadItemWithId:(NSString *)itemId
                      name:(NSString *)name
                  progress:(void (^)(MWMCategoryProgress progress))progress
                completion:(void (^)(NSError * error))completion
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [MWMBookmarksManager manager].catalogObservers = [NSMutableDictionary dictionary];
    auto onDownloadStarted = [](std::string const & catId)
    {
      auto observer = [MWMBookmarksManager manager].catalogObservers[@(catId.c_str())];
      if (observer)
        [observer onDownloadStart];
    };
    auto onDownloadFinished = [](std::string const & catId, platform::RemoteFile::Result const & result)
    {
      auto observer = [MWMBookmarksManager manager].catalogObservers[@(catId.c_str())];
      if (observer)
      {
        [observer onDownloadComplete:result.m_status];
        if (result.m_status != platform::RemoteFile::Status::Ok) {
          [[MWMBookmarksManager manager].catalogObservers removeObjectForKey:observer.categoryId];
        }
      }
    };
    auto onImportStarted = [](std::string const & catId)
    {
      auto observer = [MWMBookmarksManager manager].catalogObservers[@(catId.c_str())];
      if (observer)
        [observer onImportStart];
    };
    auto onImportFinished = [](std::string const & catId, bool successful)
    {
      auto observer = [MWMBookmarksManager manager].catalogObservers[@(catId.c_str())];
      if (observer)
      {
        [observer onImportCompleteSuccessful:successful];
        [[MWMBookmarksManager manager].catalogObservers removeObjectForKey:observer.categoryId];
      }
    };
    GetFramework().GetBookmarkManager().SetCatalogHandlers(std::move(onDownloadStarted),
                                                           std::move(onDownloadFinished),
                                                           std::move(onImportStarted),
                                                           std::move(onImportFinished));
  });
  auto *observer = [[MWMCatalogObserver alloc] init];
  observer.categoryId = itemId;
  observer.progressBlock = progress;
  observer.completionBlock = completion;
  [[MWMBookmarksManager manager].catalogObservers setObject:observer forKey:itemId];
  GetFramework().GetBookmarkManager().DownloadFromCatalogAndImport(itemId.UTF8String, name.UTF8String);
}

+ (BOOL)isCategoryFromCatalog:(MWMMarkGroupID)groupId
{
  return GetFramework().GetBookmarkManager().IsCategoryFromCatalog(groupId);
}

+ (NSArray<MWMCatalogCategory *> *)categoriesFromCatalog
{
  NSMutableArray * result = [NSMutableArray array];
  auto const & list = GetFramework().GetBookmarkManager().GetBmGroupsIdList();
  for (auto const & groupId : list)
  {
    if ([self isCategoryFromCatalog:groupId])
    {
      kml::CategoryData categoryData = GetFramework().GetBookmarkManager().GetCategoryData(groupId);
      UInt64 bookmarksCount = [self getCategoryMarksCount:groupId] + [self getCategoryTracksCount:groupId];
      MWMCatalogCategory * category = [[MWMCatalogCategory alloc] initWithCategoryData:categoryData
                                                                       bookmarksCount:bookmarksCount];
      [result addObject:category];
    }
  }
  return [result copy];
}

+ (NSInteger)getCatalogDownloadsCount
{
  return GetFramework().GetBookmarkManager().GetCatalog().GetDownloadingCount();
}

+ (BOOL)isCategoryDownloading:(NSString *)itemId
{
  return GetFramework().GetBookmarkManager().GetCatalog().IsDownloading(itemId.UTF8String);
}

+ (BOOL)hasCategoryDownloaded:(NSString *)itemId
{
  return GetFramework().GetBookmarkManager().GetCatalog().HasDownloaded(itemId.UTF8String);
}

@end
