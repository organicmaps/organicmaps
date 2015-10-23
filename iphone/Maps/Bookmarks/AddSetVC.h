#import <UIKit/UIKit.h>
#import "TableViewController.h"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(int)setIndex;

@end

@interface AddSetVC : TableViewController

@property (weak, nonatomic) id<AddSetVCDelegate> delegate;

@end
