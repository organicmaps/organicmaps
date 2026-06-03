#import <Foundation/Foundation.h>

#import "BookmarksCategoryLoadingResult.h"
#import "MWMTypes.h"
NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BookmarksObserver)
@protocol MWMBookmarksObserver <NSObject>
@optional
- (void)onBookmarksCategoryLoadingStarted;
- (void)onBookmarksCategoryLoadingFinished:(NSArray<BookmarksCategoryLoadingResult *> *)results;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
- (void)onRecentlyDeletedBookmarksCategoriesChanged;
- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId;
- (void)onTrackDeleted:(MWMTrackID)trackId;
@end

@protocol BookmarksObservable <NSObject>
- (void)addObserver:(id<MWMBookmarksObserver>)observer;
- (void)removeObserver:(id<MWMBookmarksObserver>)observer;
@end

NS_ASSUME_NONNULL_END
