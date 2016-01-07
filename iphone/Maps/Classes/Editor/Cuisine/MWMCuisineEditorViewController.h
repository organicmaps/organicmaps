#import "ViewController.h"

@protocol MWMCuisineEditorProtocol <NSObject>

- (NSSet<NSString *> *)getCuisines;
- (void)setCuisines:(NSSet<NSString *> *)cuisines;

@end

@interface MWMCuisineEditorViewController : ViewController

@property (weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
