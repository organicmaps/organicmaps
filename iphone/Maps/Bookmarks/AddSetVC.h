#import "MWMTableViewController.h"

#include "kml/type_utils.hpp"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithCategoryId:(kml::MarkGroupId)categoryId;

@end

@interface AddSetVC : MWMTableViewController

@property (weak, nonatomic) id<AddSetVCDelegate> delegate;

@end
