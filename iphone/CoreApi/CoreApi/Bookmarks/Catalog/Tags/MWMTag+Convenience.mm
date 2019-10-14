#import "MWMTag+Convenience.h"

@implementation MWMTag (Convenience)

- (instancetype)initWithTagData:(BookmarkCatalog::Tag const &)tagData
{
  self = [super init];
  if (self)
  {
    self.tagId = @(tagData.m_id.c_str());
    self.name = @(tagData.m_name.c_str());
    self.color = [UIColor colorWithRed:std::get<0>(tagData.m_color)
                                 green:std::get<1>(tagData.m_color)
                                  blue:std::get<2>(tagData.m_color)
                                 alpha:1.0];
  }
  
  return self;
}

@end
