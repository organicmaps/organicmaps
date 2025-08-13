#import "Product+Core.h"
#import "ProductsConfiguration+Core.h"

@implementation ProductsConfiguration (Core)

- (nonnull instancetype)init:(products::ProductsConfig const &)config
{
  NSMutableArray<Product *> * products = [[NSMutableArray<Product *> alloc] initWithCapacity:config.products.size()];
  for (auto const & product : config.products)
    [products addObject:[[Product alloc] init:product]];
  self = [self
      initWithPlacePagePrompt:[NSString
                                  stringWithCString:(config.placePagePrompt ? config.placePagePrompt->c_str() : "")
                                           encoding:NSUTF8StringEncoding]
                     products:products];
  return self;
}

@end
