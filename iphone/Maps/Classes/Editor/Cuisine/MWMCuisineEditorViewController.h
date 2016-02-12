#import "MWMTableViewController.h"

@protocol MWMCuisineEditorProtocol <NSObject>

@property (nonatomic) NSSet<NSString *> * cuisines;

@end

@interface MWMCuisineEditorViewController : MWMTableViewController

@property (weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
