#import <UIKit/UIKit.h>
#import "CircleView.h"
#import <QuartzCore/QuartzCore.h>

@interface CircleView()

@property(nonatomic) UIColor *circleColor;
@property(nonatomic) UIImage *image;

@end

@implementation CircleView

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color {
  return [self initWithFrame:frame andColor:color andImageName:nil];
}

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color andImageName:(nullable NSString *)imageName {
  self = [super initWithFrame:frame];
  if (self)
  {
    _circleColor = color;
    if (imageName)
      _image = [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", @"ic_bm_", [imageName lowercaseString]]];
    self.opaque = NO;
  }
  return self;
}

- (void)drawRect:(CGRect)rect {
  CGContextRef ctx = UIGraphicsGetCurrentContext();
  CGContextAddEllipseInRect(ctx, rect);
  CGContextSetFillColor(ctx, CGColorGetComponents(self.circleColor.CGColor));
  CGContextFillPath(ctx);

  if (self.image)
    [self.image drawInRect:CGRectMake(3, 3, rect.size.width - 6, rect.size.height - 6)];
}

+ (UIView *)createViewWithCircleDiameter:(CGFloat)diameter
                                andColor:(UIColor *)color
                            andImageName:(nullable NSString *)imageName {
  UIView *circleView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, diameter, diameter)];
  circleView.backgroundColor = UIColor.clearColor;
  CircleView *circle = [[self alloc] initWithFrame:CGRectMake(0.5, 0.5, diameter - 1, diameter - 1)
                                          andColor:color
                                      andImageName:imageName];
  [circleView addSubview:circle];
  return circleView;
}

+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color {
  UIView *circle = [self createViewWithCircleDiameter:diameter andColor:color andImageName:nil];
  return [self imageWithView:circle];
}

+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andImageName:(NSString *)imageName {
  UIView *circle = [self createViewWithCircleDiameter:diameter andColor:color andImageName:imageName];
  return [self imageWithView:circle];
}

+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andSubview:(UIView *)view {
  UIView *circle = [self createViewWithCircleDiameter:diameter andColor:color andImageName:nil];
  [circle addSubview:view];
  return [self imageWithView:circle];
}

+ (UIImage *)imageWithView:(UIView *)view {
  UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size];

  UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
    [view.layer renderInContext:rendererContext.CGContext];
  }];

  return image;
}

@end
