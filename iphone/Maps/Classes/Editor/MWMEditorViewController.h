#import "MWMTableViewController.h"
#include "indexer/editable_map_object.hpp"

struct FeatureID;

@interface MWMEditorViewController : MWMTableViewController

@property (nonatomic) BOOL isCreating;

- (void)setFeatureToEdit:(FeatureID const &)fid;
- (void)setEditableMapObject:(osm::EditableMapObject const &)emo;

@end
