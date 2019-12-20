#import "MWMFrameworkObserver.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MWMFrameworkStorageObserver<MWMFrameworkObserver>

- (void)processCountryEvent:(NSString *)countryId;

@optional

- (void)processCountry:(NSString *)countryId
       downloadedBytes:(uint64_t)downloadedBytes
            totalBytes:(uint64_t)totalBytes;

@end

NS_ASSUME_NONNULL_END
