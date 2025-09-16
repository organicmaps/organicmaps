#import "Product+Core.h"

@implementation Product (Core)

- (nonnull instancetype)init:(products::ProductsConfig::Product const &)product
{
  self = [self initWithTitle:[NSString stringWithCString:product.title.c_str() encoding:NSUTF8StringEncoding]
                        link:[NSString stringWithCString:product.link.c_str() encoding:NSUTF8StringEncoding]];
  return self;
}

@end
