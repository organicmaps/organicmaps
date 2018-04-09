#import "MWMDefaultAlert.h"

@interface MWMDefaultAlert (Protected)

+ (nonnull instancetype)defaultAlertWithTitle:(nonnull NSString *)title
                                      message:(nullable NSString *)message
                             rightButtonTitle:(nonnull NSString *)rightButtonTitle
                              leftButtonTitle:(nullable NSString *)leftButtonTitle
                            rightButtonAction:(nullable MWMVoidBlock)action
                              statisticsEvent:(nullable NSString *)statisticsEvent;

@property(copy, nonatomic, readonly, nullable) MWMVoidBlock rightButtonAction;

@end
