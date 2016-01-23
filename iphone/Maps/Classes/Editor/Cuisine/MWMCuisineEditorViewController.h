#import "ViewController.h"

@protocol MWMCuisineEditorProtocol <NSObject>

@property (nonatomic) NSSet<NSString *> * cuisines;

@end

@interface MWMCuisineEditorViewController : ViewController

@property (weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
