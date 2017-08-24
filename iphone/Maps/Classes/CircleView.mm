#import <UIKit/UIKit.h>
#import "CircleView.h"
#import <QuartzCore/QuartzCore.h>

@interface CircleView()

@property (nonatomic) UIColor * circleColor;

@end

@implementation CircleView

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color
{
  self = [super initWithFrame:frame];
  if (self)
  {
    _circleColor = color;
    self.opaque = NO;
  }
  return self;
}

- (void)drawRect:(CGRect)rect
{
  CGContextRef ctx = UIGraphicsGetCurrentContext();
  CGContextAddEllipseInRect(ctx, rect);
  CGContextSetFillColor(ctx, CGColorGetComponents(self.circleColor.CGColor));
  CGContextFillPath(ctx);
}

+ (UIView *)createViewWithCircleDiameter:(CGFloat)diameter andColor:(UIColor *)color
{
  UIView * circleView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, diameter, diameter)];
  circleView.backgroundColor = UIColor.clearColor;
  CircleView * circle = [[self alloc] initWithFrame:CGRectMake(0.5, 0.5, diameter - 1, diameter - 1) andColor:color];
  [circleView addSubview:circle];
  return circleView;
}

+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color
{
  UIView * circle = [self createViewWithCircleDiameter:diameter andColor:color];
  return [self imageWithView:circle];
}

+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andSubview:(UIView *)view
{
  UIView * circle = [self createViewWithCircleDiameter:diameter andColor:color];
  [circle addSubview:view];
  return [self imageWithView:circle];
}


+ (UIImage *)imageWithView:(UIView *)view
{
  UIGraphicsBeginImageContextWithOptions(view.bounds.size, NO, 0.0);
  CGContextRef context = UIGraphicsGetCurrentContext();
  [view.layer renderInContext:context];

  UIImage * img = UIGraphicsGetImageFromCurrentImageContext();

  UIGraphicsEndImageContext();

  return img;
}

@end
