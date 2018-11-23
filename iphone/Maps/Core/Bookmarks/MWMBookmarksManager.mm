#import "MWMBookmarksManager.h"
#import "AppInfo.h"
#import "MWMCatalogCategory+Convenience.h"
#import "MWMTag+Convenience.h"
#import "MWMTagGroup+Convenience.h"
#import "MWMCatalogObserver.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "coding/internal/file_data.hpp"

#include "base/stl_helpers.hpp"

#include <utility>

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

@interface MWMBookmarksManager ()

@property(nonatomic, readonly) BookmarkManager & bm;

@property(nonatomic) NSHashTable<id<MWMBookmarksObserver>> * observers;
@property(nonatomic) BOOL areBookmarksLoaded;
@property(nonatomic) NSURL * shareCategoryURL;

@property(nonatomic) NSMutableDictionary<NSString *, MWMCatalogObserver*> * catalogObservers;

@end

@implementation MWMBookmarksManager

+ (instancetype)sharedManager
{
  static MWMBookmarksManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

- (BookmarkManager &)bm
{
  return GetFramework().GetBookmarkManager();
}

- (void)addObserver:(id<MWMBookmarksObserver>)observer
{
  [self.observers addObject:observer];
}

- (void)removeObserver:(id<MWMBookmarksObserver>)observer
{
  [self.observers removeObject:observer];
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [NSHashTable<id<MWMBookmarksObserver>> weakObjectsHashTable];
    [self registerBookmarksObserver];
    [self registerCatalogObservers];
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
      [self loopObservers:^(id<MWMBookmarksObserver> observer) {
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
      [self loopObservers:^(id<MWMBookmarksObserver> observer) {
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
  self.bm.SetAsyncLoadingCallbacks(std::move(bookmarkCallbacks));
}

- (void)registerCatalogObservers
{
  self.catalogObservers = [NSMutableDictionary dictionary];
  auto onDownloadStarted = [self](std::string const & serverCatId)
  {
    auto observer = self.catalogObservers[@(serverCatId.c_str())];
    if (observer)
      [observer onDownloadStart];
  };
  auto onDownloadFinished = [self](std::string const & serverCatId, BookmarkCatalog::DownloadResult result)
  {
    auto observer = self.catalogObservers[@(serverCatId.c_str())];
    if (observer)
    {
      [observer onDownloadComplete:result];
      if (result != BookmarkCatalog::DownloadResult::Success) {
        [self.catalogObservers removeObjectForKey:observer.categoryId];
      }
    }
  };
  auto onImportStarted = [self](std::string const & serverCatId)
  {
    auto observer = self.catalogObservers[@(serverCatId.c_str())];
    if (observer)
      [observer onImportStart];
  };
  auto onImportFinished = [self](std::string const & serverCatId, kml::MarkGroupId categoryId, bool successful)
  {
    auto observer = self.catalogObservers[@(serverCatId.c_str())];
    if (observer)
    {
      [observer onImportCompleteSuccessful:successful forCategoryId:categoryId];
      [self.catalogObservers removeObjectForKey:observer.categoryId];
    }
  };
  auto onUploadStarted = [self](kml::MarkGroupId originCategoryId)
  {
    auto observer = self.catalogObservers[[NSString stringWithFormat:@"%lld", originCategoryId]];
    if (observer)
      [observer onUploadStart];
  };
  auto onUploadFinished = [self](BookmarkCatalog::UploadResult uploadResult,std::string const & description,
                             kml::MarkGroupId originCategoryId, kml::MarkGroupId resultCategoryId)
  {
    auto observer = self.catalogObservers[[NSString stringWithFormat:@"%lld", originCategoryId]];
    if (observer)
    {
      NSURL * url = [self sharingUrlForCategoryId:resultCategoryId];
      [observer onUploadComplete:uploadResult withUrl:url];
      [self.catalogObservers removeObjectForKey:observer.categoryId];
    }
  };
  self.bm.SetCatalogHandlers(std::move(onDownloadStarted),
                             std::move(onDownloadFinished),
                             std::move(onImportStarted),
                             std::move(onImportFinished),
                             std::move(onUploadStarted),
                             std::move(onUploadFinished));
}

#pragma mark - Bookmarks loading

- (BOOL)areBookmarksLoaded
{
  return _areBookmarksLoaded;
}

- (void)loadBookmarks
{
  auto onSynchronizationStarted = [self](Cloud::SynchronizationType type)
  {
    if (type == Cloud::SynchronizationType::Backup)
    {
      [Statistics logEvent:kStatBookmarksSyncStarted];
    }
    else
    {
      [self loopObservers:^(id<MWMBookmarksObserver> observer) {
        if ([observer respondsToSelector:@selector(onRestoringStarted)])
          [observer onRestoringStarted];
      }];
    }
  };
  
  auto onSynchronizationFinished = [self](Cloud::SynchronizationType type, Cloud::SynchronizationResult result,
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

    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onSynchronizationFinished:)])
        [observer onSynchronizationFinished:static_cast<MWMSynchronizationResult>(base::Key(result))];
    }];
  };
  
  auto onRestoreRequested = [self](Cloud::RestoringRequestResult result, std::string const & deviceName,
                               uint64_t backupTimestampInMs)
  {
    auto const res = static_cast<MWMRestoringRequestResult>(base::Key(result));
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

    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onRestoringRequest:deviceName:backupDate:)])
        [observer onRestoringRequest:res deviceName:@(deviceName.c_str()) backupDate:date];
    }];
  };
  
  auto onRestoredFilesPrepared = [self]
  {
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onRestoringFilesPrepared)])
        [observer onRestoringFilesPrepared];
    }];
  };
  
  self.bm.SetCloudHandlers(std::move(onSynchronizationStarted), std::move(onSynchronizationFinished),
                      std::move(onRestoreRequested), std::move(onRestoredFilesPrepared));
  self.bm.LoadBookmarks();
}

#pragma mark - Categories

- (BOOL)isCategoryEditable:(MWMMarkGroupID)groupId {
  return self.bm.IsEditableCategory(groupId);
}

- (BOOL)isCategoryNotEmpty:(MWMMarkGroupID)groupId {
  return self.bm.HasBmCategory(groupId) &&
         (self.bm.GetUserMarkIds(groupId).size() + self.bm.GetTrackIds(groupId).size());
}

- (MWMGroupIDCollection)groupsIdList
{
  auto const & list = self.bm.GetBmGroupsIdList();
  NSMutableArray<NSNumber *> * collection = @[].mutableCopy;
  for (auto const & groupId : list)
  {
    if ([self isCategoryEditable:groupId])
      [collection addObject:@(groupId)];
  }
  return collection.copy;
}

- (NSString *)getCategoryName:(MWMMarkGroupID)groupId
{
  return @(self.bm.GetCategoryName(groupId).c_str());
}

- (uint64_t)getCategoryMarksCount:(MWMMarkGroupID)groupId
{
  return self.bm.GetUserMarkIds(groupId).size();
}

- (uint64_t)getCategoryTracksCount:(MWMMarkGroupID)groupId
{
  return self.bm.GetTrackIds(groupId).size();
}

- (MWMCategoryAccessStatus)getCategoryAccessStatus:(MWMMarkGroupID)groupId
{
  switch (self.bm.GetCategoryData(groupId).m_accessRules)
  {
    case kml::AccessRules::Local:
      return MWMCategoryAccessStatusLocal;
    case kml::AccessRules::Public:
      return MWMCategoryAccessStatusPublic;
    case kml::AccessRules::DirectLink:
      return MWMCategoryAccessStatusPrivate;
    case kml::AccessRules::P2P:
    case kml::AccessRules::Paid:
    case kml::AccessRules::Count:
      return MWMCategoryAccessStatusOther;
  }
}

- (NSString *)getCategoryDescription:(MWMMarkGroupID)groupId
{
  return @(kml::GetDefaultStr(self.bm.GetCategoryData(groupId).m_description).c_str());
}

- (MWMMarkGroupID)createCategoryWithName:(NSString *)name
{
  auto groupId = self.bm.CreateBookmarkCategory(name.UTF8String);
  self.bm.SetLastEditedBmCategory(groupId);
  return groupId;
}

- (void)setCategory:(MWMMarkGroupID)groupId name:(NSString *)name
{
  self.bm.GetEditSession().SetCategoryName(groupId, name.UTF8String);
}

- (void)setCategory:(MWMMarkGroupID)groupId description:(NSString *)name
{
  self.bm.GetEditSession().SetCategoryDescription(groupId, name.UTF8String);
}

- (BOOL)isCategoryVisible:(MWMMarkGroupID)groupId
{
  return self.bm.IsVisible(groupId);
}

- (void)setCategory:(MWMMarkGroupID)groupId isVisible:(BOOL)isVisible
{
  self.bm.GetEditSession().SetIsVisible(groupId, isVisible);
}

- (void)setUserCategoriesVisible:(BOOL)isVisible {
  self.bm.SetAllCategoriesVisibility(BookmarkManager::CategoryFilterType::Private, isVisible);
}

- (void)setCatalogCategoriesVisible:(BOOL)isVisible {
  self.bm.SetAllCategoriesVisibility(BookmarkManager::CategoryFilterType::Public, isVisible);
}

- (void)deleteCategory:(MWMMarkGroupID)groupId
{
  self.bm.GetEditSession().DeleteBmCategory(groupId);
  [self loopObservers:^(id<MWMBookmarksObserver> observer) {
    if ([observer respondsToSelector:@selector(onBookmarksCategoryDeleted:)])
      [observer onBookmarksCategoryDeleted:groupId];
  }];
}

- (void)deleteBookmark:(MWMMarkID)bookmarkId
{
  self.bm.GetEditSession().DeleteBookmark(bookmarkId);
  [self loopObservers:^(id<MWMBookmarksObserver> observer) {
    if ([observer respondsToSelector:@selector(onBookmarkDeleted:)])
      [observer onBookmarkDeleted:bookmarkId];
  }];
}

- (BOOL)checkCategoryName:(NSString *)name
{
  return !self.bm.IsUsedCategoryName(name.UTF8String);
}

#pragma mark - Category sharing

- (void)shareCategory:(MWMMarkGroupID)groupId
{
  self.bm.PrepareFileForSharing(groupId, [self](auto sharingResult)
  {
    MWMBookmarksShareStatus status;
    switch (sharingResult.m_code)
    {
    case BookmarkManager::SharingResult::Code::Success:
    {
      self.shareCategoryURL = [NSURL fileURLWithPath:@(sharingResult.m_sharingPath.c_str())
                                            isDirectory:NO];
      ASSERT(self.shareCategoryURL, ("Invalid share category url"));
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
    
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onBookmarksCategoryFilePrepared:)])
        [observer onBookmarksCategoryFilePrepared:status];
    }];
  });
}

- (NSURL *)shareCategoryURL
{
  NSAssert(_shareCategoryURL != nil, @"Invalid share category url");
  return _shareCategoryURL;
}

- (void)finishShareCategory
{
  if (!self.shareCategoryURL)
    return;
  
  base::DeleteFileX(self.shareCategoryURL.path.UTF8String);
  self.shareCategoryURL = nil;
}

#pragma mark - klm conversion

- (NSUInteger)filesCountForConversion
{
  return self.bm.GetKmlFilesCountForConversion();
}

- (void)convertAll
{
  self.bm.ConvertAllKmlFiles([self](bool success) {
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onConversionFinish:)])
        [observer onConversionFinish:success];
    }];
  });
}

#pragma mark - Cloud sync

- (NSDate *)lastSynchronizationDate
{
  auto timestampInMs = self.bm.GetLastSynchronizationTimestampInMs();
  if (timestampInMs == 0)
    return nil;
  return [NSDate dateWithTimeIntervalSince1970:timestampInMs / 1000];
}

- (BOOL)isCloudEnabled
{
  return self.bm.IsCloudEnabled();
}

- (void)setCloudEnabled:(BOOL)enabled
{
  self.bm.SetCloudEnabled(enabled);
}

- (void)requestRestoring
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
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onRestoringRequest:deviceName:backupDate:)])
        [observer onRestoringRequest:MWMRestoringRequestResultNoInternet deviceName:nil backupDate:nil];
    }];
    return;
  }

  self.bm.RequestCloudRestoring();
}

- (void)applyRestoring
{
  self.bm.ApplyCloudRestoring();
}

- (void)cancelRestoring
{
  [Statistics logEvent:kStatBookmarksRestoreProposalCancel];
  self.bm.CancelCloudRestoring();
}

#pragma mark - Notifications

- (void)setNotificationsEnabled:(BOOL)enabled
{
  self.bm.SetNotificationsEnabled(enabled);
}

- (BOOL)areNotificationsEnabled
{
  return self.bm.AreNotificationsEnabled();
}

#pragma mark - Catalog

- (NSURL *)catalogFrontendUrl
{
  NSString * urlString = @(self.bm.GetCatalog().GetFrontendUrl().c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (NSURL *)sharingUrlForCategoryId:(MWMMarkGroupID)groupId
{
  NSString * urlString = @(self.bm.GetCategoryCatalogDeeplink(groupId).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (void)downloadItemWithId:(NSString *)itemId
                      name:(NSString *)name
                  progress:(ProgressBlock)progress
                completion:(DownloadCompletionBlock)completion
{
  auto observer = [[MWMCatalogObserver alloc] init];
  observer.categoryId = itemId;
  observer.progressBlock = progress;
  observer.downloadCompletionBlock = completion;
  [self.catalogObservers setObject:observer forKey:itemId];
  self.bm.DownloadFromCatalogAndImport(itemId.UTF8String, name.UTF8String);
}

- (void)uploadAndGetDirectLinkCategoryWithId:(MWMMarkGroupID)itemId
                                    progress:(ProgressBlock)progress
                                  completion:(UploadCompletionBlock)completion
{
  [self registerUploadObserverForCategoryWithId:itemId progress:progress completion:completion];
  GetFramework().GetBookmarkManager().UploadToCatalog(itemId, kml::AccessRules::DirectLink);
}

- (void)uploadAndPublishCategoryWithId:(MWMMarkGroupID)itemId
                              progress:(ProgressBlock)progress
                            completion:(UploadCompletionBlock)completion
{
  [self registerUploadObserverForCategoryWithId:itemId progress:progress completion:completion];
  GetFramework().GetBookmarkManager().UploadToCatalog(itemId, kml::AccessRules::Public);
}

- (void)registerUploadObserverForCategoryWithId:(MWMMarkGroupID)itemId
                                       progress:(ProgressBlock)progress
                                     completion:(UploadCompletionBlock)completion
{
  auto observer = [[MWMCatalogObserver alloc] init];
  observer.categoryId = [NSString stringWithFormat:@"%lld", itemId];
  observer.progressBlock = progress;
  observer.uploadCompletionBlock = completion;
  [self.catalogObservers setObject:observer forKey:observer.categoryId];
}

- (BOOL)isCategoryFromCatalog:(MWMMarkGroupID)groupId
{
  return self.bm.IsCategoryFromCatalog(groupId);
}

- (NSArray<MWMCatalogCategory *> *)categoriesFromCatalog
{
  NSMutableArray * result = [NSMutableArray array];
  auto const & list = self.bm.GetBmGroupsIdList();
  for (auto const & groupId : list)
  {
    if (![self isCategoryEditable:groupId])
    {
      kml::CategoryData categoryData = self.bm.GetCategoryData(groupId);
      uint64_t bookmarksCount = [self getCategoryMarksCount:groupId] + [self getCategoryTracksCount:groupId];
      MWMCatalogCategory * category = [[MWMCatalogCategory alloc] initWithCategoryData:categoryData
                                                                       bookmarksCount:bookmarksCount];
      [result addObject:category];
    }
  }
  return [result copy];
}

- (NSInteger)getCatalogDownloadsCount
{
  return self.bm.GetCatalog().GetDownloadingCount();
}

- (BOOL)isCategoryDownloading:(NSString *)itemId
{
  return self.bm.GetCatalog().IsDownloading(itemId.UTF8String);
}

- (BOOL)hasCategoryDownloaded:(NSString *)itemId
{
  return self.bm.GetCatalog().HasDownloaded(itemId.UTF8String);
}

- (void)loadTags:(LoadTagsCompletionBlock)completionBlock {
  auto onTagsCompletion = [completionBlock](bool success, BookmarkCatalog::TagGroups const & tagGroups)
  {
    if (success)
    {
      NSMutableArray * groups = [NSMutableArray new];
      for (auto const & groupData : tagGroups)
      {
        MWMTagGroup * tagGroup = [[MWMTagGroup alloc] initWithGroupData:groupData];
        [groups addObject:tagGroup];
      }
      
      completionBlock([groups copy]);
    } else
      completionBlock(nil);
  };
  
  self.bm.GetCatalog().RequestTagGroups([[AppInfo sharedInfo] languageId].UTF8String, std::move(onTagsCompletion));
}

- (void)setCategory:(MWMMarkGroupID)groupId tags:(NSArray<MWMTag *> *)tags
{
  vector<string> tagIds;
  for (MWMTag * tag in tags)
  {
    tagIds.push_back(tag.tagId.UTF8String);
  }
  
  self.bm.GetEditSession().SetCategoryTags(groupId, tagIds);
}

- (void)setCategory:(MWMMarkGroupID)groupId authorType:(MWMCategoryAuthorType)author
{
  switch (author)
  {
    case MWMCategoryAuthorTypeLocal:
      self.bm.GetEditSession().SetCategoryCustomProperty(groupId, @"author_type".UTF8String, @"local".UTF8String);
      break;
      
    case MWMCategoryAuthorTypeTraveler:
      self.bm.GetEditSession().SetCategoryCustomProperty(groupId, @"author_type".UTF8String, @"tourist".UTF8String);
  }
}

#pragma mark - Helpers

- (void)loopObservers:(void (^)(id<MWMBookmarksObserver> observer))block
{
  for (id<MWMBookmarksObserver> observer in [self.observers copy])
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

@end
