#import "MWMInputValidator.h"

@interface UITextField (RuntimeAttributes)

@property (copy, nonatomic) NSString * localizedPlaceholder;
@property (nonatomic) MWMInputValidator * validator;
@property (nonatomic, readonly) BOOL isValid;

@end
