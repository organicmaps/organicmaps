#include "map/discovery/discovery_client_params.hpp"

@protocol MWMDiscoveryTapDelegate<NSObject>

- (void)tapOnItem:(discovery::ItemType const)type atIndex:(size_t const)index;
- (void)routeToItem:(discovery::ItemType const)type atIndex:(size_t const)index;
- (void)openURLForItem:(discovery::ItemType const)type;

@end
