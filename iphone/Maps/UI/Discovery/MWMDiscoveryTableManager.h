#include "map/discovery/discovery_client_params.hpp"

#include <functional>
#include <vector>

namespace discovery
{
NSString * StatProvider(ItemType const type);
}  // namespace discovery

@class MWMDiscoveryControllerViewModel;

typedef MWMDiscoveryControllerViewModel *(^MWMGetModelCallback)(void);

@protocol MWMDiscoveryTapDelegate;

@interface MWMDiscoveryTableManager : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView
                    canUseNetwork:(BOOL)canUseNetwork
                         delegate:(id<MWMDiscoveryTapDelegate>)delegate
                            model:(MWMGetModelCallback)modelCallback;

- (void)loadItems:(std::vector<discovery::ItemType> const &)types;
- (void)reloadItem:(discovery::ItemType const)type;
- (void)errorAtItem:(discovery::ItemType const)type;
- (void)reloadGuidesIfNeeded;

@end
