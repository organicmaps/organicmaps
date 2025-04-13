#import "MWMViewController.h"

#include <string>

namespace osm
{
class EditableMapObject;
}  // namespace osm

@protocol MWMObjectsCategorySelectorDelegate <NSObject>

- (void)reloadObject:(osm::EditableMapObject const &)object;

@end

@interface MWMObjectsCategorySelectorController : MWMViewController

@property (weak, nonatomic) id<MWMObjectsCategorySelectorDelegate> delegate;

- (void)setSelectedCategory:(std::string const &)type;

@end
