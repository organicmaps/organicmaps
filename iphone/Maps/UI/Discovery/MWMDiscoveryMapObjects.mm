#import "MWMDiscoveryMapObjects.h"

@interface MWMDiscoveryMapObjects() {
  search::Results m_results;
  std::vector<search::ProductInfo> m_productInfos;
  m2::PointD m_viewportCenter;
}

@end

@implementation MWMDiscoveryMapObjects

- (instancetype)initWithSearchResults:(search::Results const &)results
                         productInfos:(std::vector<search::ProductInfo> const &)productInfos
                       viewPortCenter:(m2::PointD const &)viewportCenter {
  self = [super init];
  if (self) {
    m_results = results;
    m_productInfos = productInfos;
    m_viewportCenter = viewportCenter;
  }
  return self;
}

- (search::Result const &)searchResultAtIndex:(NSUInteger)index {
  CHECK_LESS(index, m_results.GetCount(), ("Incorrect index:", index));
  return m_results[index];
}

- (search::ProductInfo const &)productInfoAtIndex:(NSUInteger)index {
  CHECK_LESS(index, m_productInfos.size(), ("Incorrect index:", index));
  return m_productInfos[index];
}

- (m2::PointD const &)viewPortCenter {
  return m_viewportCenter;
}

- (NSUInteger)count {
  return m_results.GetCount();
}

@end
