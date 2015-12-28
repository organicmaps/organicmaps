#import "MWMInputValidator.h"

@interface MWMInputValidatorFactory : NSObject

+ (MWMInputValidator *)validator:(NSString *)validator;

@end
