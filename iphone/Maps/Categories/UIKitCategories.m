
#import "UIKitCategories.h"

@implementation NSObject (Optimized)

+ (NSString *)className
{
  return NSStringFromClass(self);
}

- (void)performAfterDelay:(NSTimeInterval)delay block:(void (^)(void))block
{
  [self performSelector:@selector(fireBlockAfterDelay:) withObject:[block copy] afterDelay:delay];
}

- (void)fireBlockAfterDelay:(void (^)(void))block
{
  block();
}

@end


@implementation UIColor (HexColor)

+ (UIColor *)colorWithColorCode:(NSString *)hexString
{
  NSString * cleanString = [hexString stringByReplacingOccurrencesOfString:@"#" withString:@""];

  if (cleanString.length == 6)
    cleanString = [cleanString stringByAppendingString:@"ff"];

  unsigned int baseValue;
  [[NSScanner scannerWithString:cleanString] scanHexInt:&baseValue];

  float red = ((baseValue >> 24) & 0xFF) / 255.f;
  float green = ((baseValue >> 16) & 0xFF) / 255.f;
  float blue = ((baseValue >> 8) & 0xFF) / 255.f;
  float alpha = ((baseValue >> 0) & 0xFF) / 255.f;

  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

+ (UIColor *)applicationBackgroundColor
{
  return [UIColor colorWithColorCode:@"efeff4"];
}

+ (UIColor *)applicationColor
{
  return [UIColor colorWithColorCode:@"15c783"];
}

+ (UIColor *)navigationBarColor
{
  return [UIColor colorWithColorCode:@"15c783"];
}

@end


@implementation UIView (Coordinates)

- (void)setMidX:(CGFloat)midX
{
  self.center = CGPointMake(midX, self.center.y);
}

- (CGFloat)midX
{
  return self.center.x;
}

- (void)setMidY:(CGFloat)midY
{
  self.center = CGPointMake(self.center.x, midY);
}

- (CGFloat)midY
{
  return self.center.y;
}

- (void)setOrigin:(CGPoint)origin
{
  self.frame = CGRectMake(origin.x, origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGPoint)origin
{
  return self.frame.origin;
}

- (void)setMinX:(CGFloat)minX
{
  self.frame = CGRectMake(minX, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minX
{
  return self.frame.origin.x;
}

- (void)setMinY:(CGFloat)minY
{
  self.frame = CGRectMake(self.frame.origin.x, minY, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minY
{
  return self.frame.origin.y;
}

- (void)setMaxX:(CGFloat)maxX
{
  self.frame = CGRectMake(maxX - self.frame.size.width, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)maxX
{
  return self.frame.origin.x + self.frame.size.width;
}

- (void)setMaxY:(CGFloat)maxY
{
  self.frame = CGRectMake(self.frame.origin.x, maxY - self.frame.size.height, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)maxY
{
  return self.frame.origin.y + self.frame.size.height;
}

- (void)setWidth:(CGFloat)width
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, width, self.frame.size.height);
}

- (CGFloat)width
{
  return self.frame.size.width;
}

- (void)setHeight:(CGFloat)height
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, height);
}

- (CGFloat)height
{
  return self.frame.size.height;
}

- (CGSize)size
{
  return self.frame.size;
}

- (void)setSize:(CGSize)size
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, size.width, size.height);
}

+ (void)animateWithDuration:(NSTimeInterval)duration delay:(NSTimeInterval)delay damping:(double)dampingRatio initialVelocity:(double)springVelocity options:(UIViewAnimationOptions)options animations:(void (^)(void))animations completion:(void (^)(BOOL))completion
{
  if ([UIView respondsToSelector:@selector(animateWithDuration:delay:usingSpringWithDamping:initialSpringVelocity:options:animations:completion:)])
    [UIView animateWithDuration:duration delay:delay usingSpringWithDamping:dampingRatio initialSpringVelocity:springVelocity options:options animations:animations completion:completion];
  else
    [UIView animateWithDuration:(duration * dampingRatio) delay:delay options:options animations:animations completion:completion];
}

@end


@implementation UIApplication (URLs)

- (void)openProVersionFrom:(NSString *)launchPlaceName
{
  NSURL * url = [NSURL URLWithString:MAPSWITHME_PREMIUM_LOCAL_URL];
  if ([self canOpenURL:url])
  {
    [self openURL:url];
  }
  else
  {
    NSString * urlString = [NSString stringWithFormat:@"itms-apps://itunes.apple.com/app/id510623322?mt=8&at=1l3v7ya&ct=%@", launchPlaceName];
    [self openURL:[NSURL URLWithString:urlString]];
  }
}

- (void)openGuideWithName:(NSString *)guideName itunesURL:(NSString *)itunesURL
{
  NSString * guide = [[guideName stringByReplacingOccurrencesOfString:@" " withString:@""] lowercaseString];
  NSURL * url = [NSURL URLWithString:[NSString stringWithFormat:@"guidewithme-%@://", guide]];
  if ([self canOpenURL:url])
    [self openURL:url];
  else
    [self openURL:[NSURL URLWithString:itunesURL]];
}

@end


@implementation NSString (Size)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font
{
  if ([self respondsToSelector:@selector(boundingRectWithSize:options:attributes:context:)])
    return [self boundingRectWithSize:size options:NSStringDrawingUsesLineFragmentOrigin attributes:@{NSFontAttributeName : font} context:nil].size;
  else
    return [self sizeWithFont:font constrainedToSize:size lineBreakMode:NSLineBreakByWordWrapping];
}

@end


@implementation SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

@end


@implementation SolidTouchImageView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

@end
