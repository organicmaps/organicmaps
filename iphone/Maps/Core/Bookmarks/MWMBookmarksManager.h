#import "MWMBookmarksObserver.h"
#import "MWMTypes.h"

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
+ (void)setAllCategoriesVisible:(BOOL)isVisible;
+ (void)deleteCategory:(MWMMarkGroupID)groupId;

+ (NSURL *)beginShareCategory:(MWMMarkGroupID)groupId;
+ (void)endShareCategory:(MWMMarkGroupID)groupId;

+ (NSDate *)lastSynchronizationDate;
+ (BOOL)isCloudEnabled;
+ (void)setCloudEnabled:(BOOL)enabled;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)alloc __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +manager instead")));
+ (instancetype) new __attribute__((unavailable("call +manager instead")));

@end
