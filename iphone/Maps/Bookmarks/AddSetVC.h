#import "MWMTableViewController.h"

#include "drape_frontend/user_marks_global.hpp"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithCategoryId:(df::MarkGroupID)categoryId;

@end

@interface AddSetVC : MWMTableViewController

@property (weak, nonatomic) id<AddSetVCDelegate> delegate;

@end
