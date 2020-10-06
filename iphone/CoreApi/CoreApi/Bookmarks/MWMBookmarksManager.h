#import "MWMTypes.h"

#import "MWMBookmarksObserver.h"
#import "MWMCatalogCommon.h"
#import "MWMUTM.h"
#import "PlacePageBookmarkData.h"

@class CLLocation;
@class MWMBookmark;
@class MWMBookmarkGroup;
@class MWMBookmarksSection;
@class MWMCarPlayBookmarkObject;
@class MWMTagGroup;
@class MWMTag;
@class MWMTrack;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MWMBookmarksSortingType) {
  MWMBookmarksSortingTypeByType,
  MWMBookmarksSortingTypeByDistance,
  MWMBookmarksSortingTypeByTime
} NS_SWIFT_NAME(BookmarksSortingType);

typedef void (^LoadTagsCompletionBlock)(NSArray<MWMTagGroup *> * _Nullable tags, NSInteger maxTagsNumber);
typedef void (^PingCompletionBlock)(BOOL success);
typedef void (^ElevationPointChangedBlock)(double distance);
typedef void (^SearchBookmarksCompletionBlock)(NSArray<MWMBookmark *> *bookmarks);
typedef void (^SortBookmarksCompletionBlock)(NSArray<MWMBookmarksSection *> * _Nullable sortedSections);

NS_SWIFT_NAME(BookmarksManager)
@interface MWMBookmarksManager : NSObject

+ (MWMBookmarksManager *)sharedManager;

- (void)addObserver:(id<MWMBookmarksObserver>)observer;
- (void)removeObserver:(id<MWMBookmarksObserver>)observer;

- (BOOL)areBookmarksLoaded;
- (void)loadBookmarks;

- (BOOL)isCategoryEditable:(MWMMarkGroupID)groupId;
- (BOOL)isCategoryNotEmpty:(MWMMarkGroupID)groupId;
- (BOOL)isSearchAllowed:(MWMMarkGroupID)groupId;
- (void)prepareForSearch:(MWMMarkGroupID)groupId;
- (NSString *)getCategoryName:(MWMMarkGroupID)groupId;
- (uint64_t)getCategoryMarksCount:(MWMMarkGroupID)groupId;
- (uint64_t)getCategoryTracksCount:(MWMMarkGroupID)groupId;
- (MWMBookmarkGroupAccessStatus)getCategoryAccessStatus:(MWMMarkGroupID)groupId;
- (NSString *)getCategoryAnnotation:(MWMMarkGroupID)groupId;
- (NSString *)getCategoryDescription:(MWMMarkGroupID)groupId;
- (NSString *)getCategoryAuthorName:(MWMMarkGroupID)groupId;
- (NSString *)getCategoryAuthorId:(MWMMarkGroupID)groupId;
- (nullable NSURL *)getCategoryImageUrl:(MWMMarkGroupID)groupId;
- (BOOL)hasExtraInfo:(MWMMarkGroupID)groupId;

- (MWMMarkGroupID)createCategoryWithName:(NSString *)name;
- (void)setCategory:(MWMMarkGroupID)groupId name:(NSString *)name;
- (void)setCategory:(MWMMarkGroupID)groupId description:(NSString *)name;
- (BOOL)isCategoryVisible:(MWMMarkGroupID)groupId;
- (void)setCategory:(MWMMarkGroupID)groupId isVisible:(BOOL)isVisible;
- (void)setUserCategoriesVisible:(BOOL)isVisible;
- (void)setCatalogCategoriesVisible:(BOOL)isVisible;
- (void)deleteCategory:(MWMMarkGroupID)groupId;
- (BOOL)checkCategoryName:(NSString *)name;
- (NSArray<NSNumber *> *)availableSortingTypes:(MWMMarkGroupID)groupId hasMyPosition:(BOOL)hasMyPosition;
- (void)sortBookmarks:(MWMMarkGroupID)groupId
          sortingType:(MWMBookmarksSortingType)sortingType
             location:(CLLocation * _Nullable)location
           completion:(SortBookmarksCompletionBlock)completionBlock;
- (BOOL)hasLastSortingType:(MWMMarkGroupID)groupId;
- (MWMBookmarksSortingType)lastSortingType:(MWMMarkGroupID)groupId;
- (void)resetLastSortingType:(MWMMarkGroupID)groupId;

- (NSArray<MWMCarPlayBookmarkObject *> *)bookmarksForCategory:(MWMMarkGroupID)categoryId;
- (MWMMarkIDCollection)bookmarkIdsForCategory:(MWMMarkGroupID)categoryId;
- (void)deleteBookmark:(MWMMarkID)bookmarkId;
- (NSArray<MWMBookmark *> *)bookmarksForGroup:(MWMMarkGroupID)groupId;
- (NSArray<MWMTrack *> *)tracksForGroup:(MWMMarkGroupID)groupId;
- (void)searchBookmarksGroup:(MWMMarkGroupID)groupId
                        text:(NSString *)text
                  completion:(SearchBookmarksCompletionBlock)completion;

- (MWMTrackIDCollection)trackIdsForCategory:(MWMMarkGroupID)categoryId;

- (void)shareCategory:(MWMMarkGroupID)groupId;
- (NSURL *)shareCategoryURL;
- (void)finishShareCategory;

- (NSDate * _Nullable)lastSynchronizationDate;
- (BOOL)isCloudEnabled;
- (void)setCloudEnabled:(BOOL)enabled;
- (void)requestRestoring;
- (void)applyRestoring;
- (void)cancelRestoring;

- (NSUInteger)filesCountForConversion;
- (void)convertAll;

- (void)setNotificationsEnabled:(BOOL)enabled;
- (BOOL)areNotificationsEnabled;

- (NSURL * _Nullable)catalogFrontendUrl:(MWMUTM)utm;
- (NSURL * _Nullable)injectCatalogUTMContent:(NSURL * _Nullable)url content:(MWMUTMContent)content;
- (NSURL * _Nullable)catalogFrontendUrlPlusPath:(NSString *)path
                                            utm:(MWMUTM)utm;
- (NSURL * _Nullable)deeplinkForCategoryId:(MWMMarkGroupID)groupId;
- (NSURL * _Nullable)publicLinkForCategoryId:(MWMMarkGroupID)groupId;
- (NSURL * _Nullable)webEditorUrlForCategoryId:(MWMMarkGroupID)groupId language:(NSString *)languageCode;
- (void)downloadItemWithId:(NSString *)itemId
                      name:(NSString *)name
                  progress:(_Nullable ProgressBlock)progress
                completion:(_Nullable DownloadCompletionBlock)completion;
- (BOOL)isCategoryFromCatalog:(MWMMarkGroupID)groupId;
- (NSArray<MWMBookmarkGroup *> *)userCategories;
- (NSArray<MWMBookmarkGroup *> *)categoriesFromCatalog;
- (MWMBookmarkGroup *)categoryWithId:(MWMMarkGroupID)groupId;
- (NSInteger)getCatalogDownloadsCount;
- (BOOL)isCategoryDownloading:(NSString *)itemId;
- (BOOL)hasCategoryDownloaded:(NSString *)itemId;
- (void)updateBookmark:(MWMMarkID)bookmarkId
            setGroupId:(MWMMarkGroupID)groupId
                 title:(NSString *)title
                 color:(MWMBookmarkColor)color
           description:(NSString *)description;

- (void)loadTagsWithLanguage:(NSString * _Nullable)languageCode completion:(LoadTagsCompletionBlock)completionBlock;
- (void)setCategory:(MWMMarkGroupID)groupId tags:(NSArray<MWMTag *> *)tags;
- (void)setCategory:(MWMMarkGroupID)groupId authorType:(MWMBookmarkGroupAuthorType)author;

- (void)uploadAndGetDirectLinkCategoryWithId:(MWMMarkGroupID)itemId
                                    progress:(_Nullable ProgressBlock)progress
                                  completion:(UploadCompletionBlock)completion;

- (void)uploadAndPublishCategoryWithId:(MWMMarkGroupID)itemId
                              progress:(_Nullable ProgressBlock)progress
                            completion:(UploadCompletionBlock)completion;

- (void)uploadCategoryWithId:(MWMMarkGroupID)itemId
                    progress:(_Nullable ProgressBlock)progress
                  completion:(UploadCompletionBlock)completion;
- (void)ping:(PingCompletionBlock)callback;
- (void)checkForExpiredCategories:(MWMBoolBlock)completion;
- (void)deleteExpiredCategories;
- (void)resetExpiredCategories;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype) new __attribute__((unavailable("call +manager instead")));

- (BOOL)isGuide:(MWMMarkGroupID)groupId;
- (NSString *)getServerId:(MWMMarkGroupID)groupId;
- (MWMMarkGroupID)getGroupId:(NSString *)serverId;
- (NSString *)getGuidesIds;
- (NSString *)deviceId;
- (NSDictionary<NSString *, NSString *> *)getCatalogHeaders;

- (void)setElevationActivePoint:(double)distance trackId:(uint64_t)trackId;
- (void)setElevationActivePointChanged:(uint64_t)trackId callback:(ElevationPointChangedBlock)callback;
- (void)resetElevationActivePointChanged;
- (void)setElevationMyPositionChanged:(uint64_t)trackId callback:(ElevationPointChangedBlock)callback;
- (void)resetElevationMyPositionChanged;

@end
NS_ASSUME_NONNULL_END
