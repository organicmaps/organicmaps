@protocol MWMSearchObserver<NSObject>

@optional
- (void)onSearchStarted;
- (void)onSearchCompleted;
- (void)onSearchResultsUpdated;

@end
