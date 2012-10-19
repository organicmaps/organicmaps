#import "SearchSuggestionsCell.h"

@implementation SearchSuggestionsCell

@synthesize delegate;
@synthesize separatorColor;

- (CGRect)touchRect:(NSUInteger)imageIndex
{
  NSUInteger const count = [images count];
  if (imageIndex >= count)
    return CGRectMake(0, 0, 0, 0);

  CGRect const r = self.bounds;
  CGFloat const wBlock = r.size.width/count;
  return CGRectMake(r.origin.x + imageIndex * wBlock, r.origin.y, wBlock, r.size.height);
}

- (void)gestureTapEvent:(UITapGestureRecognizer *)gesture
{
  assert([images count] == [suggestions count]);

  CGPoint pt = [gesture locationInView:gesture.view];
  for (NSUInteger i = 0; i < [images count]; ++i)
    if (CGRectContainsPoint([self touchRect:i], pt))
    {
      [delegate onSuggestionSelected:[suggestions objectAtIndex:i]];
      break;
    }
}

- (id)initWithReuseIdentifier:(NSString *)identifier
{
  self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
  if (self)
  {
    images = [[NSMutableArray alloc] initWithCapacity:7];
    suggestions = [[NSMutableArray alloc] initWithCapacity:7];
    UITapGestureRecognizer * tapG = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(gestureTapEvent:)] autorelease];
    tapG.delaysTouchesBegan = YES;
    self.userInteractionEnabled = YES;
    self.separatorColor = [UIColor colorWithRed:224./255. green:224./255. blue:224./255. alpha:1.0];
    [self addGestureRecognizer:tapG];
    self.selectionStyle = UITableViewCellSelectionStyleNone;
  }
  return self;
}

- (void)dealloc
{
  [separatorColor release];
  [images release];
  [suggestions release];

  [super dealloc];
}

- (void)addIcon:(UIImage *)icon withSuggestion:(NSString *)suggestion
{
  [images addObject:icon];
  [suggestions addObject:suggestion];
}

- (void)drawRect:(CGRect)rect
{
  for (NSUInteger i = 0; i < [images count]; ++i)
  {
    CGRect const r = [self touchRect:i];
    // Draw frame
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetLineWidth(context, 1.0);
    CGContextSetStrokeColorWithColor(context, separatorColor.CGColor);    
    CGContextAddRect(context, r);
    CGContextStrokePath(context);
    // Draw icon
    UIImage * img = (UIImage *)[images objectAtIndex:i];
    CGPoint const pt = CGPointMake(r.origin.x + (r.size.width - img.size.width)/2,
                                   r.origin.y + (r.size.height - img.size.height)/2);
    [img drawAtPoint:pt];
  }
}

@end
