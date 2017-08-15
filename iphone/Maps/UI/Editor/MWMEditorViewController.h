#import "MWMTableViewController.h"

struct FeatureID;
namespace osm
{
class EditableMapObject;
}  // namespace osm

@interface MWMEditorViewController : MWMTableViewController

@property (nonatomic) BOOL isCreating;

- (void)setFeatureToEdit:(FeatureID const &)fid;
- (void)setEditableMapObject:(osm::EditableMapObject const &)emo;

@end
