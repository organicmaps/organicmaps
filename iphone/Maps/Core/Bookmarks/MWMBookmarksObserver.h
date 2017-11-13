@protocol MWMBookmarksObserver<NSObject>

- (void)onBookmarksLoadFinished;

@optional
- (void)onBookmarksFileLoadSuccess;

@end
