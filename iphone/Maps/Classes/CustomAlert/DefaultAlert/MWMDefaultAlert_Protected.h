#import "MWMDefaultAlert.h"

@interface MWMDefaultAlert (Protected)

+ (nonnull instancetype)defaultAlertWithTitle:(nonnull NSString *)title
                                      message:(nullable NSString *)message
                             rightButtonTitle:(nonnull NSString *)rightButtonTitle
                              leftButtonTitle:(nullable NSString *)leftButtonTitle
                            rightButtonAction:(nullable MWMVoidBlock)action
                              log:(nullable NSString *)log;

@property(copy, nonatomic, readonly, nullable) MWMVoidBlock rightButtonAction;

@end
