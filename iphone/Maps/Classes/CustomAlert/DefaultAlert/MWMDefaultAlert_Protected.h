#import "MWMDefaultAlert.h"

@interface MWMDefaultAlert (Protected)

+ (nonnull instancetype)defaultAlertWithTitle:(nonnull NSString *)title
                                      message:(nullable NSString *)message
                             rightButtonTitle:(nonnull NSString *)rightButtonTitle
                              leftButtonTitle:(nullable NSString *)leftButtonTitle
                            rightButtonAction:(nullable TMWMVoidBlock)action
                              statisticsEvent:(nonnull NSString *)statisticsEvent;

@property(copy, nonatomic, nullable) TMWMVoidBlock rightButtonAction;

@end
