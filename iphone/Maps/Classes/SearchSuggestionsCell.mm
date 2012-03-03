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

//- (void)layoutSubviews
//{
//  [super layoutSubviews];
//  CGRect r = self.contentView.bounds;
//  // Leave some experimentally choosen paddings
//  CGFloat const KPaddingX = 10.0;
//  CGFloat const KPaddingBottom = 1.0;
//
//  r.origin.x += KPaddingX;
//  r.size.width -= 2 * KPaddingX;
//  r.size.height -= KPaddingBottom;
//
//  // Labels on the right should always fit and be visible, but not more than half of the cell
//  CGFloat const w = r.size.width;
//  CGFloat const h = r.size.height;
//  CGFloat const yDelim = (int)(r.origin.y + h / 5 * 3);
//
//  CGFloat xTopDelim = (int)(r.origin.x + w / 2);
//  CGFloat xBottomDelim = xTopDelim;
//  if (featureType.text.length)
//  {
//    CGSize const typeTextSize = [featureType.text sizeWithFont:featureType.font];
//    if (xTopDelim + typeTextSize.width < r.origin.x + w)
//      xTopDelim = r.origin.x + w - typeTextSize.width - KPaddingX;
//  }
//  featureName.frame = CGRectMake(r.origin.x, r.origin.y, xTopDelim - r.origin.x, yDelim - r.origin.y);
//  featureType.frame = CGRectMake(xTopDelim, r.origin.y, r.origin.x + w - xTopDelim, yDelim - r.origin.y);
//
//  if (featureDistance.text.length)
//  {
//    CGSize const distanceTextSize = [featureDistance.text sizeWithFont:featureDistance.font];
//    if (xBottomDelim + distanceTextSize.width < r.origin.x + w)
//      xBottomDelim = r.origin.x + w - distanceTextSize.width - KPaddingX;
//  }
//  featureCountry.frame = CGRectMake(r.origin.x, yDelim, xBottomDelim - r.origin.x, r.origin.y + h - yDelim);
//  featureDistance.frame = CGRectMake(xBottomDelim, yDelim, r.origin.x + w - xBottomDelim, r.origin.y + h - yDelim);
//}

@end
