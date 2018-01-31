#import "MWMTableViewController.h"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithCategoryId:(int)categoryId;

@end

@interface AddSetVC : MWMTableViewController

@property (weak, nonatomic) id<AddSetVCDelegate> delegate;

@end
