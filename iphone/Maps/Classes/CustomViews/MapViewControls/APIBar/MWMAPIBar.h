@interface MWMAPIBar : NSObject

@property (nonatomic) BOOL isVisible;

- (nullable instancetype)initWithController:(nonnull UIViewController *)controller;

- (void)back;

@end
