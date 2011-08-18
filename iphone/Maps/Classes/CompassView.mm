#import "CompassView.h"

@implementation CompassView

@synthesize angle = m_angle;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self)
  {
    self.opaque = NO;
    self.clearsContextBeforeDrawing = YES;
  }
  return self;
}

- (void)setAngle:(float)angle
{
  m_angle = angle;
  [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
  UIBezierPath * aPath = [UIBezierPath bezierPath];
  
  float const w = rect.size.width;
  float const w2 = w / 2.0;
  float const w3 = w / 3.0;
  [aPath moveToPoint:CGPointMake(w3, w2)];
  [aPath addLineToPoint:CGPointMake(0, w2 - w3)];
  [aPath addLineToPoint:CGPointMake(w, w2)];
  [aPath addLineToPoint:CGPointMake(0, w2 + w3)];
  [aPath closePath];
  
  CGAffineTransform matrix = CGAffineTransformMakeTranslation(w2, w2);
  matrix = CGAffineTransformRotate(matrix, m_angle + M_PI_2);
  matrix = CGAffineTransformTranslate(matrix, -w2, -w2);
  [aPath applyTransform:matrix];
  
  [[UIColor blueColor] setFill];
  [aPath fill];
}

@end
