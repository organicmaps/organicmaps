#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include "geometry/point2d.hpp"

#include <vector>

NS_ASSUME_NONNULL_BEGIN

@interface MWMDiscoveryMapObjects : NSObject

- (instancetype)initWithSearchResults:(search::Results const &)results
                         productInfos:(std::vector<search::ProductInfo> const &)productInfos
                       viewPortCenter:(m2::PointD const &)viewportCenter;
- (search::Result const &)searchResultAtIndex:(NSUInteger)index;
- (search::ProductInfo const &)productInfoAtIndex:(NSUInteger)index;
- (m2::PointD const &)viewPortCenter;
- (NSUInteger)count;

@end

NS_ASSUME_NONNULL_END
