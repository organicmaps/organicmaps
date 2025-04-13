#import "MWMBookmark.h"

#include <CoreApi/Framework.h>

@interface MWMBookmark (Core)

- (instancetype)initWithMarkId:(MWMMarkID)markId bookmarkData:(Bookmark const *)bookmark;

@end
