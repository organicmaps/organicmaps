#import "MWMTypes.h"

@protocol MWMBookmarksObserver<NSObject>

@optional
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;

@end
