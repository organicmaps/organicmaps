#import "MWMMapDownloaderDataSource.h"

#include "storage/downloader_search_params.hpp"

@interface MWMMapDownloaderSearchDataSource : MWMMapDownloaderDataSource

- (instancetype)initWithSearchResults:(DownloaderSearchResults const &)results delegate:(id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate;

@end
