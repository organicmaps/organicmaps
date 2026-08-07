@class MWMAlertViewController;
@interface MWMAlert : UIView

@property(weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)locationAlertWithCancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)routingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block;
+ (MWMAlert *)locationServicesDisabledAlert;
+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block;
+ (MWMAlert *)editorViralAlert;
+ (MWMAlert *)osmAuthAlert;
+ (MWMAlert *)osmReauthAlert;
+ (MWMAlert *)createBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                             minCharacterNum:(NSUInteger)min
                                                    callback:(MWMCheckStringBlock)callback;
+ (MWMAlert *)spinnerAlertWithTitle:(NSString *)title cancel:(MWMVoidBlock)cancel;

- (void)close:(MWMVoidBlock)completion;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
