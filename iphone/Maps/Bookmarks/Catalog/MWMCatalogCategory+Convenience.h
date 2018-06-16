#import "MWMCatalogCategory.h"
#include "Framework.h"

@interface MWMCatalogCategory (Convenience)

- (instancetype)initWithCategoryData:(kml::CategoryData &)categoryData bookmarksCount:(UInt64)count;

@end
