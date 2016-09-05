#import "MWMAlert.h"

@interface MWMRoutingDisclaimerAlert : MWMAlert

+ (instancetype)alertWithInitialOrientation:(UIInterfaceOrientation)orientation
                                    okBlock:(TMWMVoidBlock)block;

@end
