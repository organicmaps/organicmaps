#import "MWMMapDownloaderDataSource.h"

#include "storage/downloader_search_params.hpp"

@interface MWMMapDownloaderSearchDataSource : MWMMapDownloaderDataSource

@property(nonatomic, readonly) BOOL isEmpty;

- (instancetype)initWithSearchResults:(DownloaderSearchResults const &)results delegate:(id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate;

@end
