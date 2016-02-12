#import "MWMTableViewController.h"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(int)setIndex;

@end

@interface AddSetVC : MWMTableViewController

@property (weak, nonatomic) id<AddSetVCDelegate> delegate;

@end
