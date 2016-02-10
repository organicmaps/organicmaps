#import "MWMMapDownloaderDataSource.h"

#include "search/result.hpp"

@interface MWMMapDownloaderSearchDataSource : MWMMapDownloaderDataSource

- (instancetype)initWithSearchResults:(search::Results const &)results;

@end
