#include "map/discovery/discovery_client_params.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace discovery
{
class DiscoveryControllerViewModel;

std::pair<NSString *, NSString *> StatCategoryAndProvider(ItemType const type);
}  // namespace discovery

using GetModelCallback = std::function<discovery::DiscoveryControllerViewModel const &()>;

@protocol MWMDiscoveryTapDelegate;

@interface MWMDiscoveryTableManager : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView
                         delegate:(id<MWMDiscoveryTapDelegate>)delegate
                            model:(GetModelCallback &&)modelCallback;

- (void)loadItems:(std::vector<discovery::ItemType> const &)types;
- (void)reloadItem:(discovery::ItemType const)type;
- (void)errorAtItem:(discovery::ItemType const)type;

@end
