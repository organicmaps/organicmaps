#import <UIKit/UIKit.h>

@interface MWMTextView : UITextView

@property(nonatomic, copy) NSString *placeholder;
@property(nonatomic, readonly) UILabel *placeholderView;

@end
