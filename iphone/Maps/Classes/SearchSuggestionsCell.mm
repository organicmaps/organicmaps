#import "SearchSuggestionsCell.h"

@implementation SearchSuggestionsCell

@synthesize delegate;

- (CGRect)imageRect:(NSUInteger)imageIndex
{
  NSUInteger const count = [images count];
  if (imageIndex >= count)
    return CGRectMake(0, 0, 0, 0);

  CGRect const r = self.bounds;
  CGFloat const wBlock = r.size.width/count;
  CGFloat const yOffset = 10;
  CGFloat const wIcon = r.size.height - 2 * yOffset;
  CGFloat const x = r.origin.x + (wBlock - wIcon)/2 + imageIndex * wBlock;
  return CGRectMake(x, r.origin.y + yOffset, wIcon, wIcon);
}

- (void)gestureTapEvent:(UITapGestureRecognizer *)gesture
{
  assert([images count] == [suggestions count]);

  CGPoint pt = [gesture locationInView:gesture.view];
  for (NSUInteger i = 0; i < [images count]; ++i)
    if (CGRectContainsPoint([self imageRect:i], pt))
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
    [self addGestureRecognizer:tapG];
  }
  return self;
}

- (void)dealloc
{
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
    CGRect const r = [self imageRect:i];
    UIImage * img = (UIImage *)[images objectAtIndex:i];
    [img drawInRect:r];
  }
}

@end
