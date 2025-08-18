#import "MWMMapSearchResult+Core.h"

@implementation MWMMapSearchResult

@end

@implementation MWMMapSearchResult (Core)

- (instancetype)initWithSearchResult:(storage::DownloaderSearchResult const &)searchResult
{
  self = [super init];
  if (self)
  {
    _countryId = @(searchResult.m_countryId.c_str());
    _matchedName = @(searchResult.m_matchedName.c_str());
  }
  return self;
}

@end
