@protocol MWMBookmarksObserver<NSObject>

@optional
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;

@end
