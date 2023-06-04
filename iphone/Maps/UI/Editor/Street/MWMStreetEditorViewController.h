#import "MWMTableViewController.h"

#include "indexer/editable_map_object.hpp"

#include <vector>

@protocol MWMStreetEditorProtocol <NSObject>

- (osm::LocalizedStreet const &)currentStreet;
- (void)setNearbyStreet:(osm::LocalizedStreet const &)street;
- (std::vector<osm::LocalizedStreet> const &)nearbyStreets;

@end

@interface MWMStreetEditorViewController : MWMTableViewController

@property (weak, nonatomic) id<MWMStreetEditorProtocol> delegate;

@end
