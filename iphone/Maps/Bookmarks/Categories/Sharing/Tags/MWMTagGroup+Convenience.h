#import "MWMTagGroup.h"
#include "map/bookmark_catalog.hpp"

@interface MWMTagGroup (Convenience)

- (instancetype)initWithGroupData:(BookmarkCatalog::TagGroup const &)groupData;

@end
