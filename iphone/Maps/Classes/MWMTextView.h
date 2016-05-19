#import <UIKit/UIKit.h>

@interface MWMTextView : UITextView

@property (copy, nonatomic) NSString * placeholder;
@property (nonatomic, readonly) UILabel * placeholderView;

@end
