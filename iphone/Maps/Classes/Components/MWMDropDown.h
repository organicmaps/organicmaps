@interface MWMDropDown : NSObject

- (instancetype)initWithSuperview:(UIView *)view;
- (void)showWithMessage:(NSString *)message;
- (void)dismiss;

- (instancetype)init __attribute__((unavailable("call -initWithSuperview: instead!")));
+ (instancetype)new __attribute__((unavailable("call -initWithSuperview: instead!")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call -initWithSuperview: instead!")));
- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithSuperview: instead!")));

@end
