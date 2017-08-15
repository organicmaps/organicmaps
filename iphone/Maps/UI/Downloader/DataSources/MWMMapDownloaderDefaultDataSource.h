#import "MWMMapDownloaderDataSource.h"

@interface MWMMapDownloaderDefaultDataSource : MWMMapDownloaderDataSource

- (instancetype)
initForRootCountryId:(NSString *)countryId
            delegate:
                (id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate
                mode:(MWMMapDownloaderMode)mode;
- (void)load;

@end
