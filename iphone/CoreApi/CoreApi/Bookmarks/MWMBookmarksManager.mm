#import "MWMBookmarksManager.h"

#import "MWMBookmark+Core.h"
#import "MWMBookmarksSection.h"
#import "MWMBookmarkGroup.h"
#import "MWMCarPlayBookmarkObject.h"
#import "MWMCatalogObserver.h"
#import "MWMTag.h"
#import "MWMTagGroup+Convenience.h"
#import "MWMTrack+Core.h"
#import "MWMUTM+Core.h"

#include "Framework.h"

#include "map/bookmarks_search_params.hpp"
#include "map/purchase.hpp"

#include "partners_api/utm.hpp"

#include "web_api/utils.hpp"

#include "coding/internal/file_data.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <utility>

static kml::PredefinedColor convertBookmarkColor(MWMBookmarkColor bookmarkColor) {
  switch (bookmarkColor) {
    case MWMBookmarkColorNone:
      return kml::PredefinedColor::None;
    case MWMBookmarkColorRed:
      return kml::PredefinedColor::Red;
    case MWMBookmarkColorBlue:
      return kml::PredefinedColor::Blue;
    case MWMBookmarkColorPurple:
      return kml::PredefinedColor::Purple;
    case MWMBookmarkColorYellow:
      return kml::PredefinedColor::Yellow;
    case MWMBookmarkColorPink:
      return kml::PredefinedColor::Pink;
    case MWMBookmarkColorBrown:
      return kml::PredefinedColor::Brown;
    case MWMBookmarkColorGreen:
      return kml::PredefinedColor::Green;
    case MWMBookmarkColorOrange:
      return kml::PredefinedColor::Orange;
    case MWMBookmarkColorDeepPurple:
      return kml::PredefinedColor::DeepPurple;
    case MWMBookmarkColorLightBlue:
      return kml::PredefinedColor::LightBlue;
    case MWMBookmarkColorCyan:
      return kml::PredefinedColor::Cyan;
    case MWMBookmarkColorTeal:
      return kml::PredefinedColor::Teal;
    case MWMBookmarkColorLime:
      return kml::PredefinedColor::Lime;
    case MWMBookmarkColorDeepOrange:
      return kml::PredefinedColor::DeepOrange;
    case MWMBookmarkColorGray:
      return kml::PredefinedColor::Gray;
    case MWMBookmarkColorBlueGray:
      return kml::PredefinedColor::BlueGray;
    case MWMBookmarkColorCount:
      return kml::PredefinedColor::Count;
  }
}

static MWMBookmarksSortingType convertSortingType(BookmarkManager::SortingType const &sortingType) {
  switch (sortingType) {
    case BookmarkManager::SortingType::ByType:
      return MWMBookmarksSortingTypeByType;
    case BookmarkManager::SortingType::ByDistance:
      return MWMBookmarksSortingTypeByDistance;
    case BookmarkManager::SortingType::ByTime:
      return MWMBookmarksSortingTypeByTime;
  }
}

static BookmarkManager::SortingType convertSortingTypeToCore(MWMBookmarksSortingType sortingType) {
  switch (sortingType) {
    case MWMBookmarksSortingTypeByType:
      return BookmarkManager::SortingType::ByType;
    case MWMBookmarksSortingTypeByDistance:
      return BookmarkManager::SortingType::ByDistance;
    case MWMBookmarksSortingTypeByTime:
      return BookmarkManager::SortingType::ByTime;
  }
}

@interface MWMBookmarksManager ()

@property(nonatomic, readonly) BookmarkManager & bm;

@property(nonatomic) NSHashTable<id<MWMBookmarksObserver>> * observers;
@property(nonatomic) BOOL areBookmarksLoaded;
@property(nonatomic) NSURL * shareCategoryURL;
@property(nonatomic) NSInteger lastSearchId;
@property(nonatomic) NSInteger lastSortId;

@property(nonatomic) NSMutableDictionary<NSString *, MWMCatalogObserver*> * catalogObservers;

@end

@implementation MWMBookmarksManager

+ (instancetype)sharedManager
{
  static MWMBookmarksManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] initManager];
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
      __strong __typeof(self) self = wSelf;
      [self loopObservers:^(id<MWMBookmarksObserver> observer) {
        if ([observer respondsToSelector:@selector(onBookmarksFileLoadSuccess)])
          [observer onBookmarksFileLoadSuccess];
      }];
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFileError = [wSelf](std::string const & filePath, bool isTemporaryFile) {
      __strong __typeof(self) self = wSelf;
      [self loopObservers:^(id<MWMBookmarksObserver> observer) {
        if ([observer respondsToSelector:@selector(onBookmarksFileLoadError)])
          [observer onBookmarksFileLoadError];
      }];
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
      [observer onUploadComplete:uploadResult];
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
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if (type == Cloud::SynchronizationType::Backup) {
        if ([observer respondsToSelector:@selector(onBackupStarted)]) {
          [observer onBackupStarted];
        }
      } else {
        if ([observer respondsToSelector:@selector(onRestoringStarted)]) {
          [observer onRestoringStarted];
        }
      }
    }];
  };
  
  auto onSynchronizationFinished = [self](Cloud::SynchronizationType type,
                                          Cloud::SynchronizationResult result,
                                          std::string const & errorStr)
  {
    [self loopObservers:^(id<MWMBookmarksObserver> observer) {
      if ([observer respondsToSelector:@selector(onSynchronizationFinished:)])
        [observer onSynchronizationFinished:static_cast<MWMSynchronizationResult>(base::Underlying(result))];
    }];
  };
  
  auto onRestoreRequested = [self](Cloud::RestoringRequestResult result,
                                   std::string const & deviceName,
                                   uint64_t backupTimestampInMs)
  {
    auto const res = static_cast<MWMRestoringRequestResult>(base::Underlying(result));
    NSDate * date = nil;

    if (result == Cloud::RestoringRequestResult::BackupExists) {
        date = [NSDate dateWithTimeIntervalSince1970:backupTimestampInMs / 1000];
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
  
  self.bm.SetCloudHandlers(std::move(onSynchronizationStarted),
                           std::move(onSynchronizationFinished),
                           std::move(onRestoreRequested),
                           std::move(onRestoredFilesPrepared));
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

- (BOOL)isSearchAllowed:(MWMMarkGroupID)groupId {
  return self.bm.IsSearchAllowed(groupId);
}

- (void)prepareForSearch:(MWMMarkGroupID)groupId {
  self.bm.PrepareForSearch(groupId);
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

- (MWMBookmarkGroupAccessStatus)getCategoryAccessStatus:(MWMMarkGroupID)groupId
{
  switch (self.bm.GetCategoryData(groupId).m_accessRules)
  {
    case kml::AccessRules::Local:
      return MWMBookmarkGroupAccessStatusLocal;
    case kml::AccessRules::Public:
      return MWMBookmarkGroupAccessStatusPublic;
    case kml::AccessRules::DirectLink:
      return MWMBookmarkGroupAccessStatusPrivate;
    case kml::AccessRules::AuthorOnly:
      return MWMBookmarkGroupAccessStatusAuthorOnly;
    case kml::AccessRules::P2P:
    case kml::AccessRules::Paid:
    case kml::AccessRules::Count:
      return MWMBookmarkGroupAccessStatusOther;
  }
}

- (NSString *)getCategoryAnnotation:(MWMMarkGroupID)groupId {
  return @(GetPreferredBookmarkStr(self.bm.GetCategoryData(groupId).m_annotation).c_str());
}

- (NSString *)getCategoryDescription:(MWMMarkGroupID)groupId
{
  return @(GetPreferredBookmarkStr(self.bm.GetCategoryData(groupId).m_description).c_str());
}

- (NSString *)getCategoryAuthorName:(MWMMarkGroupID)groupId
{
  return @(self.bm.GetCategoryData(groupId).m_authorName.c_str());
}

- (NSString *)getCategoryAuthorId:(MWMMarkGroupID)groupId
{
  return @(self.bm.GetCategoryData(groupId).m_authorId.c_str());
}

- (nullable NSURL *)getCategoryImageUrl:(MWMMarkGroupID)groupId {
  NSString *urlString = @(self.bm.GetCategoryData(groupId).m_imageUrl.c_str());
  return [NSURL URLWithString:urlString];
}

- (BOOL)hasExtraInfo:(MWMMarkGroupID)groupId {
  auto data = self.bm.GetCategoryData(groupId);
  return !data.m_description.empty() || !data.m_annotation.empty();
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

- (BOOL)checkCategoryName:(NSString *)name
{
  return !self.bm.IsUsedCategoryName(name.UTF8String);
}

- (NSArray<NSNumber *> *)availableSortingTypes:(MWMMarkGroupID)groupId hasMyPosition:(BOOL)hasMyPosition{
  auto const availableTypes = self.bm.GetAvailableSortingTypes(groupId, hasMyPosition);
  NSMutableArray *result = [NSMutableArray array];
  for (auto const &sortingType : availableTypes) {
    [result addObject:[NSNumber numberWithInteger:convertSortingType(sortingType)]];
  }
  return [result copy];
}

- (void)sortBookmarks:(MWMMarkGroupID)groupId
          sortingType:(MWMBookmarksSortingType)sortingType
             location:(CLLocation *)location
           completion:(SortBookmarksCompletionBlock)completion {
  self.bm.SetLastSortingType(groupId, convertSortingTypeToCore(sortingType));
  m2::PointD myPosition = m2::PointD::Zero();

  if (sortingType == MWMBookmarksSortingTypeByDistance) {
    if (!location) {
      completion(nil);
      return;
    }
    myPosition = mercator::FromLatLon(location.coordinate.latitude, location.coordinate.longitude);
  }

  auto const sortId = ++self.lastSortId;
  __weak auto weakSelf = self;

  BookmarkManager::SortParams sortParams;
  sortParams.m_groupId = groupId;
  sortParams.m_sortingType = convertSortingTypeToCore(sortingType);
  sortParams.m_hasMyPosition = location != nil;
  sortParams.m_myPosition = myPosition;
  sortParams.m_onResults = [weakSelf, sortId, completion] (BookmarkManager::SortedBlocksCollection &&sortedBlocks,
                                                           BookmarkManager::SortParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || sortId != self.lastSortId)
      return;

    switch (status) {
      case BookmarkManager::SortParams::Status::Completed: {
        NSMutableArray *result = [NSMutableArray array];
        for (auto const &sortedBlock : sortedBlocks) {
          NSMutableArray *bookmarks = nil;
          if (sortedBlock.m_markIds.size() > 0) {
            bookmarks = [NSMutableArray array];
            for (auto const &markId : sortedBlock.m_markIds) {
              [bookmarks addObject:[[MWMBookmark alloc] initWithMarkId:markId
                                                          bookmarkData:self.bm.GetBookmark(markId)]];
            }
          }
          NSMutableArray *tracks = nil;
          if (sortedBlock.m_trackIds.size() > 0) {
            tracks = [NSMutableArray array];
            for (auto const &trackId : sortedBlock.m_trackIds) {
              [tracks addObject:[[MWMTrack alloc] initWithTrackId:trackId trackData:self.bm.GetTrack(trackId)]];
            }
          }
          [result addObject:[[MWMBookmarksSection alloc] initWithTitle:@(sortedBlock.m_blockName.c_str())
                                                             bookmarks:bookmarks
                                                                tracks:tracks]];
        }
        completion([result copy]);
        break;
      }
      case BookmarkManager::SortParams::Status::Cancelled:
        completion(nil);
        break;
    }
  };

  self.bm.GetSortedCategory(sortParams);
}

- (BOOL)hasLastSortingType:(MWMMarkGroupID)groupId {
  BookmarkManager::SortingType st;
  return self.bm.GetLastSortingType(groupId, st);
}

- (MWMBookmarksSortingType)lastSortingType:(MWMMarkGroupID)groupId {
  BookmarkManager::SortingType st;
  self.bm.GetLastSortingType(groupId, st);
  return convertSortingType(st);
}

- (void)resetLastSortingType:(MWMMarkGroupID)groupId {
  self.bm.ResetLastSortingType(groupId);
}

#pragma mark - Bookmarks

- (NSArray<MWMCarPlayBookmarkObject *> *)bookmarksForCategory:(MWMMarkGroupID)categoryId
{
  NSMutableArray<MWMCarPlayBookmarkObject *> * result = [NSMutableArray array];
  auto const & bookmarkIds = self.bm.GetUserMarkIds(categoryId);
  for (auto bookmarkId : bookmarkIds)
  {
    MWMCarPlayBookmarkObject *bookmark = [[MWMCarPlayBookmarkObject alloc] initWithBookmarkId:bookmarkId];
    [result addObject:bookmark];
  }
  return [result copy];
}

- (MWMMarkIDCollection)bookmarkIdsForCategory:(MWMMarkGroupID)categoryId {
  auto const &bookmarkIds = self.bm.GetUserMarkIds(categoryId);
  NSMutableArray<NSNumber *> *collection = [[NSMutableArray alloc] initWithCapacity:bookmarkIds.size()];
  for (auto bookmarkId : bookmarkIds)
    [collection addObject:@(bookmarkId)];
  return [collection copy];
}

- (void)deleteBookmark:(MWMMarkID)bookmarkId
{
  self.bm.GetEditSession().DeleteBookmark(bookmarkId);
  [self loopObservers:^(id<MWMBookmarksObserver> observer) {
    if ([observer respondsToSelector:@selector(onBookmarkDeleted:)])
      [observer onBookmarkDeleted:bookmarkId];
  }];
}

- (NSArray<MWMBookmark *> *)bookmarksForGroup:(MWMMarkGroupID)groupId {
  auto const &bookmarkIds = self.bm.GetUserMarkIds(groupId);
  NSMutableArray *result = [NSMutableArray array];
  for (auto bookmarkId : bookmarkIds) {
    [result addObject:[[MWMBookmark alloc] initWithMarkId:bookmarkId bookmarkData:self.bm.GetBookmark(bookmarkId)]];
  }
  return [result copy];
}

- (void)searchBookmarksGroup:(MWMMarkGroupID)groupId
                        text:(NSString *)text
                  completion:(SearchBookmarksCompletionBlock)completion {
  search::BookmarksSearchParams searchParams;
  searchParams.m_query = text.UTF8String;
  searchParams.m_groupId = groupId;

  auto const searchId = ++self.lastSearchId;
  __weak auto weakSelf = self;
  searchParams.m_onStarted = [] {};
  searchParams.m_onResults = [weakSelf, searchId, completion](search::BookmarksSearchParams::Results const &results,
                                                              search::BookmarksSearchParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || searchId != self.lastSearchId)
      return;

    auto filteredResults = results;
    self.bm.FilterInvalidBookmarks(filteredResults);

    NSMutableArray *result = [NSMutableArray array];
    for (auto bookmarkId : filteredResults)
      [result addObject:[[MWMBookmark alloc] initWithMarkId:bookmarkId bookmarkData:self.bm.GetBookmark(bookmarkId)]];

    completion([result copy]);
  };

  GetFramework().GetSearchAPI().SearchInBookmarks(searchParams);
}


#pragma mark - Tracks

- (MWMTrackIDCollection)trackIdsForCategory:(MWMMarkGroupID)categoryId {
  auto const &trackIds = self.bm.GetTrackIds(categoryId);
  NSMutableArray<NSNumber *> *collection = [[NSMutableArray alloc] initWithCapacity:trackIds.size()];
  for (auto trackId : trackIds)
    [collection addObject:@(trackId)];
  return [collection copy];
}

- (NSArray<MWMTrack *> *)tracksForGroup:(MWMMarkGroupID)groupId {
  auto const &trackIds = self.bm.GetTrackIds(groupId);
  NSMutableArray *result = [NSMutableArray array];
  for (auto trackId : trackIds) {
    [result addObject:[[MWMTrack alloc] initWithTrackId:trackId trackData:self.bm.GetTrack(trackId)]];
  }
  return [result copy];
}

- (NSArray<MWMBookmarkGroup *> *)collectionsForGroup:(MWMMarkGroupID)groupId {
  auto const &collectionIds = self.bm.GetChildrenCollections(groupId);
  NSMutableArray *result = [NSMutableArray array];
  for (auto collectionId : collectionIds) {
    [result addObject:[[MWMBookmarkGroup alloc] initWithCategoryId:collectionId bookmarksManager:self]];
  }
  return [result copy];
}

- (NSArray<MWMBookmarkGroup *> *)categoriesForGroup:(MWMMarkGroupID)groupId {
  auto const &categoryIds = self.bm.GetChildrenCategories(groupId);
  NSMutableArray *result = [NSMutableArray array];
  for (auto categoryId : categoryIds) {
    [result addObject:[[MWMBookmarkGroup alloc] initWithCategoryId:categoryId bookmarksManager:self]];
  }
  return [result copy];
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
  if (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE)
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

- (NSURL *)catalogFrontendUrl:(MWMUTM)utm
{
  NSString * urlString = @(self.bm.GetCatalog().GetFrontendUrl(toUTM(utm)).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (NSURL * _Nullable)injectCatalogUTMContent:(NSURL * _Nullable)url content:(MWMUTMContent)content {
  if (!url)
    return nil;
  NSString * urlString = @(InjectUTMContent(std::string(url.absoluteString.UTF8String),
                                            toUTMContent(content)).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (NSURL * _Nullable)catalogFrontendUrlPlusPath:(NSString *)path
                                            utm:(MWMUTM)utm
{
  NSString * urlString = @(self.bm.GetCatalog().GetFrontendUrl(toUTM(utm)).c_str());
  return urlString ? [NSURL URLWithString:[urlString stringByAppendingPathComponent:path]] : nil;
}

- (NSURL *)deeplinkForCategoryId:(MWMMarkGroupID)groupId {
  NSString * urlString = @(self.bm.GetCategoryCatalogDeeplink(groupId).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (NSURL *)publicLinkForCategoryId:(MWMMarkGroupID)groupId {
  NSString *urlString = @(self.bm.GetCategoryCatalogPublicLink(groupId).c_str());
  return urlString ? [NSURL URLWithString:urlString] : nil;
}

- (NSURL *)webEditorUrlForCategoryId:(MWMMarkGroupID)groupId language:(NSString *)languageCode {
  auto serverId = self.bm.GetCategoryServerId(groupId);
  NSString *urlString = @(self.bm.GetCatalog().GetWebEditorUrl(serverId, languageCode.UTF8String).c_str());
  return [NSURL URLWithString:urlString];
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

- (void)uploadCategoryWithId:(MWMMarkGroupID)itemId
                    progress:(ProgressBlock)progress
                  completion:(UploadCompletionBlock)completion
{
  [self registerUploadObserverForCategoryWithId:itemId progress:progress completion:completion];
  GetFramework().GetBookmarkManager().UploadToCatalog(itemId, kml::AccessRules::AuthorOnly);
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

- (NSArray<MWMBookmarkGroup *> *)userCategories
{
  NSMutableArray<MWMBookmarkGroup *> * result = [NSMutableArray array];
  auto const & list = self.bm.GetBmGroupsIdList();
  for (auto const & groupId : list)
  {
    if ([self isCategoryEditable:groupId])
      [result addObject:[self categoryWithId:groupId]];
  }
  return [result copy];
}

- (NSArray<MWMBookmarkGroup *> *)categoriesFromCatalog
{
  NSMutableArray<MWMBookmarkGroup *>  * result = [NSMutableArray array];
  auto const & list = self.bm.GetBmGroupsIdList();
  for (auto const & groupId : list)
  {
    if (![self isCategoryEditable:groupId])
      [result addObject:[self categoryWithId:groupId]];
  }
  return [result copy];
}

- (MWMBookmarkGroup *)categoryWithId:(MWMMarkGroupID)groupId {
  return [[MWMBookmarkGroup alloc] initWithCategoryId:groupId bookmarksManager:self];
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

- (void)updateBookmark:(MWMMarkID)bookmarkId
            setGroupId:(MWMMarkGroupID)groupId
                 title:(NSString *)title
                 color:(MWMBookmarkColor)color
           description:(NSString *)description {
  auto const currentGroupId = self.bm.GetBookmark(bookmarkId)->GetGroupId();
  auto editSession = self.bm.GetEditSession();
  if (groupId != kml::kInvalidMarkGroupId) {
    editSession.MoveBookmark(bookmarkId, currentGroupId, groupId);
  }

  auto bookmark = editSession.GetBookmarkForEdit(bookmarkId);
  if (!bookmark)
    return;

  auto kmlColor = convertBookmarkColor(color);
  if (kmlColor != bookmark->GetColor()) {
    self.bm.SetLastEditedBmColor(kmlColor);
  }

  bookmark->SetColor(kmlColor);
  bookmark->SetDescription(description.UTF8String);
  if (title.UTF8String != bookmark->GetPreferredName()) {
    bookmark->SetCustomName(title.UTF8String);
  }
}

- (void)loadTagsWithLanguage:(NSString *)languageCode completion:(LoadTagsCompletionBlock)completionBlock {
  auto onTagsCompletion = [completionBlock](bool success, BookmarkCatalog::TagGroups const & tagGroups, uint32_t maxTagsCount)
  {
    if (success)
    {
      NSMutableArray * groups = [NSMutableArray new];
      for (auto const & groupData : tagGroups)
      {
        MWMTagGroup * tagGroup = [[MWMTagGroup alloc] initWithGroupData:groupData];
        [groups addObject:tagGroup];
      }
      
      completionBlock([groups copy], maxTagsCount);
    } else
      completionBlock(nil, 0);
  };
  
  self.bm.GetCatalog().RequestTagGroups(languageCode.UTF8String, std::move(onTagsCompletion));
}

- (void)setCategory:(MWMMarkGroupID)groupId tags:(NSArray<MWMTag *> *)tags
{
  std::vector<std::string> tagIds;
  for (MWMTag * tag in tags)
  {
    tagIds.push_back(tag.tagId.UTF8String);
  }
  
  self.bm.GetEditSession().SetCategoryTags(groupId, tagIds);
}

- (void)setCategory:(MWMMarkGroupID)groupId authorType:(MWMBookmarkGroupAuthorType)author
{
  switch (author)
  {
    case MWMBookmarkGroupAuthorTypeLocal:
      self.bm.GetEditSession().SetCategoryCustomProperty(groupId, @"author_type".UTF8String, @"local".UTF8String);
      break;
    case MWMBookmarkGroupAuthorTypeTraveler:
      self.bm.GetEditSession().SetCategoryCustomProperty(groupId, @"author_type".UTF8String, @"tourist".UTF8String);
  }
}

- (void)ping:(PingCompletionBlock)callback {
  self.bm.GetCatalog().Ping([callback] (bool success) {
    callback(success);
  });
}

- (void)checkForExpiredCategories:(MWMBoolBlock)completion {
  self.bm.CheckExpiredCategories([completion] (bool hasExpiredCategories) {
    completion(hasExpiredCategories);
  });
}

- (void)deleteExpiredCategories {
  self.bm.DeleteExpiredCategories();
}

- (void)resetExpiredCategories {
  self.bm.ResetExpiredCategories();
}

- (BOOL)isGuide:(MWMMarkGroupID)groupId {
  auto const & data = self.bm.GetCategoryData(groupId);
  return BookmarkManager::IsGuide(data.m_accessRules);
}

- (NSString *)getServerId:(MWMMarkGroupID)groupId {
  return @(self.bm.GetCategoryServerId(groupId).c_str());
}

- (MWMMarkGroupID)getGroupId:(NSString *)serverId {
  return self.bm.GetCategoryIdByServerId(serverId.UTF8String);
}

- (NSString *)getGuidesIds {
  auto const guides = self.bm.GetCategoriesFromCatalog(std::bind(&BookmarkManager::IsGuide, std::placeholders::_1));
  return @(strings::JoinStrings(guides.begin(), guides.end(), ',').c_str());
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

- (NSString *)deviceId {
  return @(web_api::DeviceId().c_str());
}

- (NSDictionary<NSString *, NSString *> *)getCatalogHeaders {
  NSMutableDictionary<NSString *, NSString *> *result = [NSMutableDictionary dictionary];

  for (auto const &header : self.bm.GetCatalog().GetHeaders())
    [result setObject:@(header.second.c_str()) forKey:@(header.first.c_str())];

  return [result copy];
}

- (void)setElevationActivePoint:(double)distance trackId:(uint64_t)trackId {
  self.bm.SetElevationActivePoint(trackId, distance);
}

- (void)setElevationActivePointChanged:(uint64_t)trackId callback:(ElevationPointChangedBlock)callback {
  __weak __typeof(self) ws = self;
  self.bm.SetElevationActivePointChangedCallback([callback, trackId, ws] () {
    callback(ws.bm.GetElevationActivePoint(trackId));
  });
}

- (void)resetElevationActivePointChanged {
  self.bm.SetElevationActivePointChangedCallback(nullptr);
}

- (void)setElevationMyPositionChanged:(uint64_t)trackId callback:(ElevationPointChangedBlock)callback {
  __weak __typeof(self) ws = self;
  self.bm.SetElevationMyPositionChangedCallback([callback, trackId, ws] () {
    callback(ws.bm.GetElevationMyPosition(trackId));
  });
}

- (void)resetElevationMyPositionChanged {
  self.bm.SetElevationMyPositionChangedCallback(nullptr);
}


@end
