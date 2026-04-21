#import "PlacePageRoute.h"

@implementation PlacePageRoute

- (instancetype)initWithRef:(NSString *)ref
                       from:(NSString *)from
                         to:(NSString *)to
                      relId:(uint32_t)relId
                      color:(nullable UIColor *)color
{
  self = [super init];
  if (self)
  {
    _ref = ref;
    _from = from;
    _to = to;
    _relId = relId;
    _color = color;
  }
  return self;
}

+ (instancetype)routeWithRef:(NSString *)ref
                        from:(NSString *)from
                          to:(NSString *)to
                       relId:(uint32_t)relId
                       color:(nullable UIColor *)color
{
  return [[self alloc] initWithRef:ref from:from to:to relId:relId color:color];
}

@end
