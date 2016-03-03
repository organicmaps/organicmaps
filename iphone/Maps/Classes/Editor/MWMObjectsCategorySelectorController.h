#include "indexer/editable_map_object.hpp"

@protocol MWMObjectsCategorySelectorDelegate <NSObject>

- (void)reloadObject:(osm::EditableMapObject const &)object;

@end

#import "MWMTableViewController.h"

#include "std/string.hpp"

@interface MWMObjectsCategorySelectorController : MWMTableViewController

@property (weak, nonatomic) id<MWMObjectsCategorySelectorDelegate> delegate;

- (void)setSelectedCategory:(string const &)category;

@end
