#import "CompassView.h"

#define DEFAULTCOLOR 168.0/255.0

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
    _color = [[UIColor alloc] initWithRed:DEFAULTCOLOR green:DEFAULTCOLOR blue:DEFAULTCOLOR alpha:1.0];
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
    /*
           bottom
         ___________
         \    |    /
          \   |   /
           \  |--/--- heights
            \ | /
             \|/

     */

    UIBezierPath * aPath = [UIBezierPath bezierPath];

    float const w = rect.size.width;
    float const w2 = w / 2.0;
    float const w3 = w / 3.0;
    float const heights = w * 0.85;
    float const halfBottom = (sqrt(w2 * w2 - (heights - w2) * (heights - w2)));
    float const margin = w - heights;

    [aPath moveToPoint:CGPointMake(w3, w2)];
    [aPath addLineToPoint:CGPointMake(margin, w2 - halfBottom)];
    [aPath addLineToPoint:CGPointMake(w, w2)];
    [aPath addLineToPoint:CGPointMake(margin, w2 + halfBottom)];
    [aPath closePath];

    CGAffineTransform matrix = CGAffineTransformMakeTranslation(w2, w2);
    matrix = CGAffineTransformRotate(matrix, -angle);
    matrix = CGAffineTransformTranslate(matrix, -w2 + 0.1 * w2, -w2 + 0.1 * w2);
    matrix = CGAffineTransformScale(matrix, 0.9, 0.9);
    [aPath applyTransform:matrix];

    [self.color setFill];
    [aPath fill];
  }
  // Do not draw anything if showArrow property is not set
}

-(void)dealloc
{
  self.color = nil;
  [super dealloc];
}

@end
