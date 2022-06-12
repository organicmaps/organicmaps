#import "MWMCarPlaySearchService.h"
#import "MWMCarPlaySearchResultObject.h"
#import "MWMSearch.h"

API_AVAILABLE(ios(12.0))
@interface MWMCarPlaySearchService ()<MWMSearchObserver>
@property(strong, nonatomic, nullable) void (^completionHandler)(NSArray<MWMCarPlaySearchResultObject *> *searchResults);
@property(strong, nonatomic, nullable) NSString *lastQuery;
@property(strong, nonatomic, nullable) NSString *inputLocale;
@property(strong, nonatomic, readwrite) NSArray<MWMCarPlaySearchResultObject *> *lastResults;

@end

@implementation MWMCarPlaySearchService

- (instancetype)init {
  self = [super init];
  if (self) {
    [MWMSearch addObserver:self];
    self.lastResults = @[];
  }
  return self;
}

- (void)searchText:(NSString *)text
    forInputLocale:(NSString *)inputLocale
 completionHandler:(void (^)(NSArray<MWMCarPlaySearchResultObject *> *searchResults))completionHandler {
  self.lastQuery = text;
  self.inputLocale = inputLocale;
  self.lastResults = @[];
  self.completionHandler = completionHandler;
  /// @todo Didn't find pure category request in CarPlay.
  [MWMSearch searchQuery:text forInputLocale:inputLocale withCategory:NO];
}

- (void)saveLastQuery {
  if (self.lastQuery != nil && self.inputLocale != nil) {
    [MWMSearch saveQuery:self.lastQuery forInputLocale:self.inputLocale];
  }
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted {
  void (^completionHandler)(NSArray<MWMCarPlaySearchResultObject *> *searchResults) = self.completionHandler;
  if (completionHandler == nil) { return; }
  
  NSMutableArray<MWMCarPlaySearchResultObject *> *results = [NSMutableArray array];
  NSInteger count = [MWMSearch resultsCount];
  for (NSInteger row = 0; row < count; row++) {
    MWMCarPlaySearchResultObject *result = [[MWMCarPlaySearchResultObject alloc] initForRow:row];
    if (result != nil) { [results addObject:result]; }
  }
  
  self.lastResults = results;
  completionHandler(results);
  self.completionHandler = nil;
}

@end
