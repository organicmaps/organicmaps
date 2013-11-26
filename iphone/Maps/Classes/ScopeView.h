
#import <UIKit/UIKit.h>

@interface ScopeView : UIView

- (instancetype)initWithFrame:(CGRect)frame segmentedControl:(UISegmentedControl *)segmentedControl;

@property (readonly) UISegmentedControl * segmentedControl;

@end
