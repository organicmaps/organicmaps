#import <UIKit/UIKit.h>

@class AddSetVC;
@protocol AddSetVCDelegate <NSObject>

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(size_t)setIndex;

@end

@interface AddSetVC : UITableViewController <UITextFieldDelegate>

@property (nonatomic, weak) id <AddSetVCDelegate> delegate;

@end
