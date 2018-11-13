#import "MWMTag.h"
#include "map/bookmark_catalog.hpp"

@interface MWMTag (Convenience)

- (instancetype)initWithTagData:(BookmarkCatalog::Tag const &)tagData;

@end
