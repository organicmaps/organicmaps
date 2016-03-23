#import "MWMMapDownloaderDataSource.h"

@interface MWMMapDownloaderDefaultDataSource : MWMMapDownloaderDataSource

- (instancetype)initForRootCountryId:(NSString *)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate;
- (void)reload;

@end
