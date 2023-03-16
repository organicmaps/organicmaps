#import "MWMMapSearchResult.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMMapSearchResult (Core)

- (instancetype)initWithSearchResult:(storage::DownloaderSearchResult const &)searchResult;

@end

NS_ASSUME_NONNULL_END
