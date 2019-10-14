#import "MWMTagGroup+Convenience.h"
#import "MWMTag+Convenience.h"

@implementation MWMTagGroup (Convenience)

- (instancetype)initWithGroupData:(BookmarkCatalog::TagGroup const &)groupData
{
  self = [super init];
  if (self)
  {
    self.name = [NSString stringWithUTF8String:groupData.m_name.c_str()];
    
    NSMutableArray * tags = [NSMutableArray new];
    for (auto const & tagData : groupData.m_tags) {
      MWMTag * tagObj = [[MWMTag alloc] initWithTagData:tagData];
      [tags addObject:tagObj];
    }
    
    self.tags = tags;
  }
  
  return self;
}

@end
