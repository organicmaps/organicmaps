// Default drop down which dismiss automaticaly after 3 seconds.
@interface MWMDropDown : NSObject

- (instancetype)initWithSuperview:(UIView *)view;
- (void)showWithMessage:(NSString *)message;

- (instancetype)init __attribute__((unavailable("call -initWithSuperview: instead!")));
+ (instancetype)new __attribute__((unavailable("call -initWithSuperview: instead!")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call -initWithSuperview: instead!")));
- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithSuperview: instead!")));

@end
