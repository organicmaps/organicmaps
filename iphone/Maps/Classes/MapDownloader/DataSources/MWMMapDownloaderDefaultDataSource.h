#import "MWMMapDownloaderDataSource.h"

@interface MWMMapDownloaderDefaultDataSource : MWMMapDownloaderDataSource

- (instancetype)initForRootCountryId:(NSString *)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate mode:(TMWMMapDownloaderMode)mode;
- (void)load;

@end
