#import "MWMMapDownloaderDataSource.h"

@interface MWMMapDownloaderDefaultDataSource : MWMMapDownloaderDataSource

@property (nonatomic, readonly) BOOL needFullReload;

- (instancetype)initForRootCountryId:(storage::TCountryId)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate;
- (void)reload;
- (std::vector<NSInteger>)getReloadSections;

@end
