#import <UIKit/UIKit.h>
#import "TableViewController.h"

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(int)setIndex;

@end

@interface AddSetVC : TableViewController <UITextFieldDelegate>

@property (nonatomic, weak) id <AddSetVCDelegate> delegate;

@end
