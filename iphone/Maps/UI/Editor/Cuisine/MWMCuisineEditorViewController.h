#import "MWMViewController.h"

#include "std/string.hpp"
#include "std/vector.hpp"

@protocol MWMCuisineEditorProtocol <NSObject>

- (vector<string>)getSelectedCuisines;
- (void)setSelectedCuisines:(vector<string> const &)cuisines;

@end

@interface MWMCuisineEditorViewController : MWMViewController

@property (weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
