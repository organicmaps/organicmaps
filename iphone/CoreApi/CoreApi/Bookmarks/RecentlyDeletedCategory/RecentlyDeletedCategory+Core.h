#import "RecentlyDeletedCategory.h"

#include "kml/types.hpp"

@interface RecentlyDeletedCategory (Core)

- (instancetype)initWithCategoryData:(kml::CategoryData)data filePath:(std::string const &)filePath;

@end
