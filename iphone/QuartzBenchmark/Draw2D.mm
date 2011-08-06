#import "Draw2D.h"

#include <vector>

using namespace std;

struct PointD
{
  double x;
  double y;
  PointD() {}
  PointD(double _x, double _y) : x(_x), y(_y)
  {}
};

@implementation Draw2D

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
      m_frameCount = 0;
      m_totalTime = 0;
    }
    return self;
}

void makeStar(unsigned segCount, PointD const & c, unsigned r, vector<PointD> & v)
{
  for (unsigned i = 0; i < segCount; ++i)
  {
    double a = (rand() % 360) / 360.0 * 2 * 3.14;
    
    v[i].x = c.x + cos(a) * r;
    v[i].y = c.y + sin(a) * r;
  }
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
  CGContextRef context = UIGraphicsGetCurrentContext();

  CGAffineTransform t0 = CGContextGetCTM(context);
  t0 = CGAffineTransformInvert(t0);
  CGContextConcatCTM(context, t0);

  CGFloat scale = [UIScreen mainScreen].scale;
  CGContextScaleCTM(context, scale, scale);
  
  if (m_frameCount == 0)
    NSLog(@"Scale : %g", scale);
  
  int segCount = 10;
  vector<PointD> v(segCount);
  int starCount = 10000;

  CGFloat pat[4] = {5, 10, 4, 2};
  
  CGContextSetLineDash(context, 0, pat, 4);
  
  NSDate * start = [[NSDate alloc] init];

  CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();  
  
  int w = (int)CGRectGetWidth(self.frame);
  int h = (int)CGRectGetHeight(self.frame);

  if (m_frameCount == 0)
    NSLog(@"frameSize %d, %d", w, h);
  
  CGContextSetLineJoin(context, kCGLineJoinBevel);
  
  for (unsigned i = 0; i < starCount; ++i)
  {
    double r = rand() % 10 + 10;
    
    PointD c(rand() % w - 2 * r + r,
                 rand() % h - 2 * r + r);
    
    makeStar(segCount, c, r, v);

    CGContextSetLineWidth(context, rand() % 10 + 3);
    CGFloat components[] = {rand() % 255 / 255.0f, rand() % 255 / 255.0f, rand() % 255 / 255.0f, 1.0f};
    CGColorRef color = CGColorCreate(colorspace, components);
    CGContextSetStrokeColorWithColor(context, color);

    CGContextMoveToPoint(context, v[0].x, v[0].y);
    
    for (unsigned i = 1; i < v.size(); ++i)
      CGContextAddLineToPoint(context, v[i].x, v[i].y);

    CGContextStrokePath(context);
    CGColorRelease(color);
  }
  
  CGColorSpaceRelease(colorspace);
  
  NSDate * end = [[NSDate alloc] init];  
  
  float len = [end timeIntervalSinceDate:start];

  m_frameCount += 1;
  m_totalTime += len;
  
  NSLog(@"last frame takes %g sec, average %g sec per frame", len, m_totalTime / m_frameCount);
  
  if (m_frameCount > 5)
    return;
  
  [self drawRect:rect];
}

- (void)layoutSubviews
{
  CGFloat scaleFactor = 1.0;
  if ([self respondsToSelector:@selector(contentScaleFactor)])
    self.contentScaleFactor = scaleFactor;   
}


- (void)dealloc
{
    [super dealloc];
}

@end
