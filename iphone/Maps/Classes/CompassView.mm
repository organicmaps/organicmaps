#import "CompassView.h"

@implementation CompassView

@synthesize angle;
@synthesize showArrow;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self)
  {
    self.opaque = NO;
    self.clearsContextBeforeDrawing = YES;
    self.showArrow = NO;
  }
  return self;
}

- (void)setAngle:(float)aAngle
{
  angle = aAngle;
  [self setNeedsDisplay];
}

- (void)setShowArrow:(BOOL)showOrNot
{
  showArrow = showOrNot;
  [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
  if (showArrow)
  {
    // Draws an arrow looking to the right like this:
    // =>
    // and rotates it to given angle
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
    matrix = CGAffineTransformRotate(matrix, -angle);
    matrix = CGAffineTransformTranslate(matrix, -w2, -w2);
    [aPath applyTransform:matrix];

    // Set color: a8a8a8.
    float const c = 168.0/255.0;
    [[UIColor colorWithRed:c green:c blue:c alpha:1.0] setFill];
    [aPath fill];
  }
  // Do not draw anything if showArrow property is not set
}

@end
