#import "MWMTableViewController.h"

#include "indexer/editable_map_object.hpp"

#include "std/vector.hpp"

@protocol MWMStreetEditorProtocol <NSObject>

- (osm::LocalizedStreet const &)currentStreet;
- (void)setNearbyStreet:(osm::LocalizedStreet const &)street;
- (vector<osm::LocalizedStreet> const &)nearbyStreets;

@end

@interface MWMStreetEditorViewController : MWMTableViewController

@property (weak, nonatomic) id<MWMStreetEditorProtocol> delegate;

@end
