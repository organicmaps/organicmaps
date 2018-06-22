#import "MWMCatalogCategory+Convenience.h"

@implementation MWMCatalogCategory (Convenience)

- (instancetype)initWithCategoryData:(kml::CategoryData &)categoryData bookmarksCount:(UInt64)count
{
  self = [self init];

  if (self)
  {
    self.categoryId = categoryData.m_id;
    self.title = @(kml::GetDefaultStr(categoryData.m_name).c_str());
    self.bookmarksCount = count;
    self.visible = categoryData.m_visible;
    self.author = @(categoryData.m_authorName.c_str());
    self.annotation = @(kml::GetDefaultStr(categoryData.m_annotation).c_str());
    self.detailedAnnotation = @(kml::GetDefaultStr(categoryData.m_description).c_str());
  }
  return self;
}

@end
