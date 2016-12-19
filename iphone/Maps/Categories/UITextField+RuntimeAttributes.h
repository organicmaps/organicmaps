#import "MWMInputValidator.h"

@interface UITextField (RuntimeAttributes)

@property (nonatomic) NSString * localizedPlaceholder;
@property (nonatomic) MWMInputValidator * validator;
@property (nonatomic, readonly) BOOL isValid;

@end
