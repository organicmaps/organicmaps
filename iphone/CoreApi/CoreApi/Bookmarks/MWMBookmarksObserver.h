#import <Foundation/Foundation.h>

#import "MWMTypes.h"
NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BookmarksObserver)
@protocol MWMBookmarksObserver <NSObject>
@optional
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;
- (void)onBookmarksFileLoadError;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
- (void)onRecentlyDeletedBookmarksCategoriesChanged;
- (void)onBookmarksDeleted:(MWMMarkIDCollection)bookmarkIds;
- (void)onTracksDeleted:(MWMTrackIDCollection)trackIds;
@end

@protocol BookmarksObservable <NSObject>
- (void)addObserver:(id<MWMBookmarksObserver>)observer;
- (void)removeObserver:(id<MWMBookmarksObserver>)observer;
@end

NS_ASSUME_NONNULL_END
