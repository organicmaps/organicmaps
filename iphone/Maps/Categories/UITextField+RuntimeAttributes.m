#import "MWMInputValidatorFactory.h"
#import "UITextField+RuntimeAttributes.h"
#import <objc/runtime.h>

@implementation UITextField (RuntimeAttributes)

- (void)setLocalizedPlaceholder:(NSString *)placeholder
{
  self.placeholder = L(placeholder);
}

- (NSString *)localizedPlaceholder
{
  NSString * placeholder = self.placeholder;
  return L(placeholder);
}

- (void)setValidator:(MWMInputValidator *)validator
{
  objc_setAssociatedObject(self, @selector(validator), validator, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (MWMInputValidator *)validator
{
  return objc_getAssociatedObject(self, @selector(validator));
}

- (void)setValidatorName:(NSString *)validatorName
{
  objc_setAssociatedObject(self, @selector(validatorName), validatorName, OBJC_ASSOCIATION_COPY_NONATOMIC);
  self.validator = [MWMInputValidatorFactory validator:validatorName];
}

- (MWMInputValidator *)validatorName
{
  return objc_getAssociatedObject(self, @selector(validatorName));
}

- (BOOL)isValid
{
  return [self.validator validateInput:self];
}

@end
