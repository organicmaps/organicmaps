#import "MWMBookmarksObserver.h"
#import "MWMTypes.h"
#import "MWMCatalogCommon.h"

@class MWMCatalogCategory;

@interface MWMBookmarksManager : NSObject

+ (void)addObserver:(id<MWMBookmarksObserver>)observer;
+ (void)removeObserver:(id<MWMBookmarksObserver>)observer;

+ (BOOL)areBookmarksLoaded;
+ (void)loadBookmarks;

+ (MWMGroupIDCollection)groupsIdList;
+ (NSString *)getCategoryName:(MWMMarkGroupID)groupId;
+ (uint64_t)getCategoryMarksCount:(MWMMarkGroupID)groupId;
+ (uint64_t)getCategoryTracksCount:(MWMMarkGroupID)groupId;

+ (MWMMarkGroupID)createCategoryWithName:(NSString *)name;
+ (void)setCategory:(MWMMarkGroupID)groupId name:(NSString *)name;
+ (BOOL)isCategoryVisible:(MWMMarkGroupID)groupId;
+ (void)setCategory:(MWMMarkGroupID)groupId isVisible:(BOOL)isVisible;
+ (void)setUserCategoriesVisible:(BOOL)isVisible;
+ (void)setCatalogCategoriesVisible:(BOOL)isVisible;
+ (void)deleteCategory:(MWMMarkGroupID)groupId;

+ (void)deleteBookmark:(MWMMarkID)bookmarkId;
+ (BOOL)checkCategoryName:(NSString *)name;

+ (void)shareCategory:(MWMMarkGroupID)groupId;
+ (NSURL *)shareCategoryURL;
+ (void)finishShareCategory;

+ (NSDate *)lastSynchronizationDate;
+ (BOOL)isCloudEnabled;
+ (void)setCloudEnabled:(BOOL)enabled;

+ (NSUInteger)filesCountForConversion;
+ (void)convertAll;

+ (void)setNotificationsEnabled:(BOOL)enabled;
+ (BOOL)areNotificationsEnabled;

+ (void)requestRestoring;
+ (void)applyRestoring;
+ (void)cancelRestoring;

+ (NSURL * _Nullable)catalogFrontendUrl;
+ (NSURL * _Nullable)sharingUrlForCategoryId:(MWMMarkGroupID)groupId;
+ (void)downloadItemWithId:(NSString * _Nonnull)itemId
                      name:(NSString * _Nonnull)name
                  progress:(void (^ _Nullable)(MWMCategoryProgress progress))progress
                completion:(void (^ _Nullable)(NSError * _Nullable error))completion;
+ (BOOL)isCategoryFromCatalog:(MWMMarkGroupID)groupId;
+ (NSArray<MWMCatalogCategory *> * _Nonnull)categoriesFromCatalog;
+ (NSInteger)getCatalogDownloadsCount;
+ (BOOL)isCategoryDownloading:(NSString * _Nonnull)itemId;
+ (BOOL)hasCategoryDownloaded:(NSString * _Nonnull)itemId;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)alloc __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +manager instead")));
+ (instancetype) new __attribute__((unavailable("call +manager instead")));

@end
