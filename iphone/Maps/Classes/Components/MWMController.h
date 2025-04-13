@class MWMAlertViewController;

@protocol MWMController <NSObject>

@property (nonatomic, readonly) BOOL hasNavigationBar;

@property (nonatomic, readonly) MWMAlertViewController * alertController;

@end
