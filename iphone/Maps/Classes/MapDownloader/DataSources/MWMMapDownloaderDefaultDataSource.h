#import "MWMMapDownloaderDataSource.h"

@interface MWMMapDownloaderDefaultDataSource : MWMMapDownloaderDataSource

- (instancetype)initForRootCountryId:(storage::TCountryId)countryId;

@end
