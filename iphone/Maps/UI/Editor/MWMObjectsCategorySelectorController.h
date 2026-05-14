#import "MWMTableViewController.h"

#include "geometry/point2d.hpp"

#include <string>

namespace osm
{
class EditableMapObject;
}  // namespace osm

@protocol MWMObjectsCategorySelectorDelegate <NSObject>

- (void)reloadObject:(osm::EditableMapObject const &)object;

@end

@interface MWMObjectsCategorySelectorController : MWMTableViewController

@property(weak, nonatomic) id<MWMObjectsCategorySelectorDelegate> delegate;

- (void)setSelectedCategory:(std::string const &)type;
// Position captured when the user confirms placement. Must be set before the controller is shown;
// the picked category is created at this exact point instead of the live viewport center
// (which can drift while the user browses categories).
- (void)setCreatedPosition:(m2::PointD const &)position;

@end
