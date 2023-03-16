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
      _image = [UIImage imageNamed:imageName];
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

+ (UIImage *)createCircleImageWithFrameSize:(CGFloat)frameSize
                                andDiameter:(CGFloat)diameter
                                   andColor:(UIColor *)color
                               andImageName:(nullable NSString *)imageName {
  UIView *circleView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frameSize, frameSize)];
  circleView.backgroundColor = UIColor.clearColor;
  CircleView *circle = [[self alloc] initWithFrame:CGRectMake(0.5, 0.5, diameter - 1, diameter - 1)
                                          andColor:color
                                      andImageName:imageName];
  circle.center = circleView.center;
  [circleView addSubview:circle];
  return  [self imageWithView:circleView];
}

+ (UIImage *)createCircleImageWithFrameSize:(CGFloat)frameSize
                                andDiameter:(CGFloat)diameter
                                   andColor:(UIColor *)color {
  return [self createCircleImageWithFrameSize:frameSize andDiameter:diameter andColor:color andImageName:nil];
}

+ (UIImage *)createCircleImageWithDiameter:(CGFloat)diameter andColor:(UIColor *)color {
  return [self createCircleImageWithFrameSize:diameter andDiameter:diameter andColor:color andImageName:nil];
}

+ (UIImage *)createCircleImageWithDiameter:(CGFloat)diameter
                                  andColor:(UIColor *)color
                              andImageName:(NSString *)imageName {
  return [self createCircleImageWithFrameSize:diameter andDiameter:diameter andColor:color andImageName:imageName];
}

+ (UIImage *)imageWithView:(UIView *)view {
  UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size];

  UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
    [view.layer renderInContext:rendererContext.CGContext];
  }];

  return image;
}

@end
