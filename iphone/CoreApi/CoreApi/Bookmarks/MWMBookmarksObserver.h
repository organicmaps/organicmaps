#import <Foundation/Foundation.h>

#import "MWMTypes.h"
NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BookmarksObserver)
@protocol MWMBookmarksObserver<NSObject>
@optional
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;
- (void)onBookmarksFileLoadError;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId;
- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status;

@end
NS_ASSUME_NONNULL_END
