#import "MWMTableViewController.h"

struct FeatureID;

@interface MWMEditorViewController : MWMTableViewController

- (void)setFeatureToEdit:(FeatureID const &)fid;

@end
