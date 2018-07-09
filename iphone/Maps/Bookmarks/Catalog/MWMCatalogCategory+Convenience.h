#import "MWMCatalogCategory.h"
#include "Framework.h"

@interface MWMCatalogCategory (Convenience)

- (instancetype)initWithCategoryData:(kml::CategoryData const &)categoryData bookmarksCount:(uint64_t)count;

@end
