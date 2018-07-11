#import "MWMCatalogCategory+Convenience.h"

#include "map/bookmark_helpers.hpp"

@implementation MWMCatalogCategory (Convenience)

- (instancetype)initWithCategoryData:(kml::CategoryData const &)categoryData bookmarksCount:(uint64_t)count
{
  self = [self init];
  if (self)
  {
    self.categoryId = categoryData.m_id;
    self.title = @(GetPreferredBookmarkStr(categoryData.m_name).c_str());
    self.bookmarksCount = count;
    self.visible = categoryData.m_visible;
    self.author = @(categoryData.m_authorName.c_str());
    self.annotation = @(GetPreferredBookmarkStr(categoryData.m_annotation).c_str());
    self.detailedAnnotation = @(GetPreferredBookmarkStr(categoryData.m_description).c_str());
  }
  
  return self;
}

@end
