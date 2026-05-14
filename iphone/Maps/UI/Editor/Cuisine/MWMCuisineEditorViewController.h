#import "MWMTableViewController.h"

#include <string>
#include <vector>

@protocol MWMCuisineEditorProtocol <NSObject>

- (std::vector<std::string>)selectedCuisines;
- (void)setSelectedCuisines:(std::vector<std::string> const &)cuisines;

@end

@interface MWMCuisineEditorViewController : MWMTableViewController

@property(weak, nonatomic) id<MWMCuisineEditorProtocol> delegate;

@end
