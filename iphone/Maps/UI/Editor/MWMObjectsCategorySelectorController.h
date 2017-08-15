#import "MWMViewController.h"

#include "std/string.hpp"

namespace osm
{
class EditableMapObject;
}  // namespace osm

@protocol MWMObjectsCategorySelectorDelegate <NSObject>

- (void)reloadObject:(osm::EditableMapObject const &)object;

@end

@interface MWMObjectsCategorySelectorController : MWMViewController

@property (weak, nonatomic) id<MWMObjectsCategorySelectorDelegate> delegate;

- (void)setSelectedCategory:(string const &)category;

@end
