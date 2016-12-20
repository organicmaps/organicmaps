#import "MWMViewController.h"

#include "indexer/editable_map_object.hpp"

#include "std/string.hpp"

@protocol MWMObjectsCategorySelectorDelegate <NSObject>

- (void)reloadObject:(osm::EditableMapObject const &)object;

@end

@interface MWMObjectsCategorySelectorController : MWMViewController

@property (weak, nonatomic) id<MWMObjectsCategorySelectorDelegate> delegate;

- (void)setSelectedCategory:(string const &)category;

@end
