#import "MWMAlert.h"

@interface MWMLocationAlert : MWMAlert

+ (instancetype)alertWithCancelBlock:(MWMVoidBlock)cancelBlock;

@end
