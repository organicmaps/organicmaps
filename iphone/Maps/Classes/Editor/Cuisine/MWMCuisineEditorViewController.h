#import "TableViewController.h"

@protocol MWMCuisineEditorProtocol <NSObject>

@property (nonatomic) NSSet<NSString *> * cuisines;

@end

@interface MWMCuisineEditorViewController : TableViewController

@property (weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
