#import "BalloonView.h"
#import <QuartzCore/CALayer.h>


@implementation BalloonView

@synthesize globalPosition;
@synthesize title;
@synthesize description;
@synthesize type;
@synthesize pinImage;
@synthesize color;
@synthesize setName;
@synthesize isDisplayed;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
{
  if ((self = [super init]))
  {
    // Default bookmark pin color
    self.color = @"placemark-purple";
    self.setName = NSLocalizedString(@"my_places", @"Default bookmarks set name");
    self.pinImage = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:self.color]] autorelease];
    isDisplayed = NO;
    m_target = target;
    m_selector = selector;
  }
  return self;
}

- (void) dealloc
{
  [m_titleView release];
  self.pinImage = nil;
  self.color = nil;
  self.setName = nil;
  self.title = nil;
  self.description = nil;
  self.type = nil;
  [super dealloc];
}

- (UIImage *)createPopupImageWithName:(NSString *)name andAddress:(NSString *)address
{
  UIImage * left = [UIImage imageNamed:@"left"];
  UIImage * right = [UIImage imageNamed:@"right"];
  UIImage * middle = [UIImage imageNamed:@"middle"];
  UIImage * tail = [UIImage imageNamed:@"tail"];
  UIImage * arrow = [UIImage imageNamed:@"arrow"];

  // Calculate text width and height
  UIFont * nameFont = [UIFont boldSystemFontOfSize:[UIFont buttonFontSize]];
  UIFont * addressFont = [UIFont systemFontOfSize:[UIFont systemFontSize]];

  CGSize const defSize = CGSizeMake(arrow.size.width + tail.size.width + left.size.width + right.size.width,
                                    tail.size.height);
  CGSize const nameSize = name ? [name sizeWithFont:nameFont] : defSize;
  CGSize const addressSize = address ? [address sizeWithFont:addressFont] : defSize;

  CGFloat const minScreenWidth = MIN([UIScreen mainScreen].applicationFrame.size.width,
                                     [UIScreen mainScreen].applicationFrame.size.height);

  CGFloat const padding = 1.;

  // Generated image size
  CGFloat const height = tail.size.height;
  CGFloat const additionalPadding = padding * 3 + arrow.size.width + left.size.width + right.size.width;
  CGFloat const width = MAX(MIN(minScreenWidth, nameSize.width + additionalPadding),
                            MIN(minScreenWidth, addressSize.width + additionalPadding));

	UIGraphicsBeginImageContextWithOptions(CGSizeMake(width, height), NO, 0.0);

  // Draw background
	[left drawAtPoint:CGPointMake(0, 0)];
  [right drawAtPoint:CGPointMake(width - right.size.width, 0)];
  CGFloat const tailStartsAt = (long)(width - tail.size.width) / 2;
  [tail drawAtPoint:CGPointMake(tailStartsAt, 0)];
  [middle drawInRect:CGRectMake(left.size.width, 0, tailStartsAt - left.size.width, middle.size.height)];
  [middle drawInRect:CGRectMake(tailStartsAt + tail.size.width, 0, width - tailStartsAt - tail.size.width - right.size.width, middle.size.height)];

  // Draw text
  CGFloat const textW = width - left.size.width - right.size.width - arrow.size.width;
  CGFloat const nameTextH = left.size.height / 2;
  [[UIColor whiteColor] set];
  [name drawInRect:CGRectMake(left.size.width, nameTextH / 8, textW, nameTextH) withFont:nameFont lineBreakMode:UILineBreakModeTailTruncation];
  [[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0] set];
  [address drawInRect:CGRectMake(left.size.width, nameTextH - (nameTextH / 8), textW, nameTextH) withFont:addressFont lineBreakMode:UILineBreakModeTailTruncation];

  // Draw Arrow image
  CGFloat const arrowPadding = (left.size.height - arrow.size.height) / 2;
  [arrow drawAtPoint:CGPointMake(width - arrow.size.width - arrowPadding, arrowPadding)];

  UIImage * theImage = UIGraphicsGetImageFromCurrentImageContext();

  UIGraphicsEndImageContext();
	// return the image
	return theImage;
}

- (void) showButtonsInView:(UIView *)view atPoint:(CGPoint)pt
{
  [m_titleView release];
  m_titleView = [[UIImageView alloc] initWithImage:[self createPopupImageWithName:self.title andAddress:self.description]];
  CGSize const s = m_titleView.bounds.size;
  m_titleView.frame = CGRectMake(pt.x, pt.y, 1, 1);

  m_titleView.userInteractionEnabled = YES;
  UITapGestureRecognizer * recognizer = [[UITapGestureRecognizer alloc]
                                         initWithTarget:m_target action:m_selector];
  recognizer.numberOfTapsRequired = 1;
  recognizer.numberOfTouchesRequired = 1;
  recognizer.delaysTouchesBegan = YES;
  [m_titleView addGestureRecognizer:recognizer];
  [recognizer release];

  [view addSubview:m_titleView];

  // Animate balloon from touched point
  [UIView transitionWithView:self.pinImage duration:0.1 options:UIViewAnimationOptionCurveEaseIn
                  animations:^{ m_titleView.frame = CGRectMake(pt.x - s.width/2, pt.y - s.height, s.width, s.height);}
                  completion:nil];

}

- (void) showInView:(UIView *)view atPoint:(CGPoint)pt withBookmark:(BookmarkAndCategory)bm
{
  if (isDisplayed)
    [self hide];

  isDisplayed = YES;

  m_rawBookmark = bm;

  CGFloat const w = self.pinImage.bounds.size.width;
  CGFloat const h = self.pinImage.bounds.size.height;
  self.pinImage.frame = CGRectMake(pt.x - w/2, pt.y - h, w, h);
  // Do not show pin if we're editing existing bookmark.
  // @TODO move pin (and probably balloon drawing) to cross-platform code
  self.pinImage.hidden = (m_rawBookmark.second != 0);

  [view addSubview:self.pinImage];

  [self showButtonsInView:view atPoint:CGPointMake(pt.x, pt.y - h)];
}

- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt
{
  if (isDisplayed)
  {
    CGFloat const w1 = self.pinImage.bounds.size.width;
    CGFloat const h1 = self.pinImage.bounds.size.height;
    self.pinImage.frame = CGRectMake(pt.x - w1/2, pt.y - h1, w1, h1);

    CGFloat const w2 = m_titleView.bounds.size.width;
    CGFloat const h2 = m_titleView.bounds.size.height;
    m_titleView.frame = CGRectMake(pt.x - w2/2, pt.y - h1 - h2, w2, h2);
  }
}

- (void) hide
{
  if (isDisplayed)
  {
    isDisplayed = NO;
    [m_titleView removeFromSuperview];
    [self.pinImage removeFromSuperview];
  }
}

// Overrided property setter to update address and displayed information
- (void) setGlobalPosition:(CGPoint)pos
{
  globalPosition = pos;
  m_addressInfo.Clear();
  GetFramework().GetAddressInfo(m2::PointD(pos.x, pos.y), m_addressInfo);
  if (m_addressInfo.m_name.empty())
  {
    if (!m_addressInfo.m_types.empty())
      self.title = [NSString stringWithUTF8String:m_addressInfo.m_types[0].c_str()];
    else
      self.title = NSLocalizedString(@"dropped_pin", @"Unknown Dropped Pin title, when name can't be determined");
  }
  else
    self.title = [NSString stringWithUTF8String:m_addressInfo.m_name.c_str()];
  self.description = [NSString stringWithUTF8String:m_addressInfo.FormatAddress().c_str()];
  self.type = [NSString stringWithUTF8String:m_addressInfo.FormatTypes().c_str()];
}

// Overrided property setter to reload another pin image
- (void) setColor:(NSString *)newColor
{
  id old = color;
  color = [newColor retain];
  [old release];
  self.pinImage.image = [UIImage imageNamed:newColor];
}

- (void) updateTitle:(NSString *)newTitle
{
  if (m_titleView != nil)
    m_titleView.image = [self createPopupImageWithName:newTitle andAddress:description];
}

- (void) deleteBMHelper
{
  BookmarkCategory * cat = m_rawBookmark.first;
  Bookmark const * bm = m_rawBookmark.second;
  if (cat && bm)
  {
    for (size_t i = 0; i < cat->GetBookmarksCount(); ++i)
    {
      if (cat->GetBookmark(i) == bm)
      {
        cat->DeleteBookmark(i);
        break;
      }
    }
  }
}

- (void) addOrEditBookmark
{
  // for an "edit" operation, delete old bookmark before adding "edited" one
  [self deleteBMHelper];
  GetFramework().AddBookmark([self.setName UTF8String],
                             Bookmark(m2::PointD(self.globalPosition.x, self.globalPosition.y),
                             [self.title UTF8String], [self.color UTF8String]));
  // Clear!
  m_rawBookmark = BookmarkAndCategory(0, 0);
}

- (void) deleteBookmark
{
  [self deleteBMHelper];
  // Clear!
  m_rawBookmark = BookmarkAndCategory(0, 0);
}

@end
