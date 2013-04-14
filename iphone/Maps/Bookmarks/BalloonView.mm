#import "BalloonView.h"
#import <QuartzCore/CALayer.h>

#include "../../../platform/settings.hpp"


#define SETTINGS_LAST_BOOKMARK_SET "LastBookmarkSet"


@implementation BalloonView

@synthesize globalPosition;
@synthesize title;
@synthesize description;
//@synthesize type;
@synthesize pinImage;
@synthesize color;
@synthesize setName;
@synthesize isDisplayed;
@synthesize editedBookmark;
@synthesize isCurrentPosition;


+ (NSString *) getDefaultSetName
{
  string savedSet;
  if (Settings::Get(SETTINGS_LAST_BOOKMARK_SET, savedSet))
    return [NSString stringWithUTF8String:savedSet.c_str()];
  // Use default set name
  return NSLocalizedString(@"my_places", @"Default bookmarks set name");
}

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
{
  if ((self = [super init]))
  {
    // Default bookmark pin color
    self.color = @"placemark-red";
    self.setName = [BalloonView getDefaultSetName];
    // Load bookmarks from kml files
    GetFramework().LoadBookmarks();
    self.pinImage = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:self.color]] autorelease];
    isDisplayed = NO;
    m_target = target;
    m_selector = selector;
    m_titleView = [[UIImageView alloc] init];
    m_titleView.userInteractionEnabled = YES;
    isCurrentPosition = NO;
    UITapGestureRecognizer * recognizer = [[[UITapGestureRecognizer alloc]
                                            initWithTarget:m_target action:m_selector] autorelease];
    recognizer.numberOfTapsRequired = 1;
    recognizer.numberOfTouchesRequired = 1;
    recognizer.delaysTouchesBegan = YES;
    [m_titleView addGestureRecognizer:recognizer];

    editedBookmark = MakeEmptyBookmarkAndCategory();
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
//  self.type = nil;
  [super dealloc];
}

// Returned image SHOULD NOT be released - it's handled automatically by context
- (UIImage *)createPopupImageWithTitle:(NSString *)aTitle andDescription:(NSString *)aDescription
{
  // description becomes a title if title is absent
  if (!aTitle && aDescription)
  {
    aTitle = aDescription;
    aDescription = nil;
  }

  UIImage * left = [UIImage imageNamed:@"left"];
  UIImage * right = [UIImage imageNamed:@"right"];
  UIImage * middle = [UIImage imageNamed:@"middle"];
  UIImage * tail = [UIImage imageNamed:@"tail"];
  UIImage * arrow = [UIImage imageNamed:(IsValid(editedBookmark) ? @"arrow" : @"add")];

  // Calculate text width and height
  UIFont * titleFont = [UIFont boldSystemFontOfSize:[UIFont buttonFontSize]];
  UIFont * descriptionFont = [UIFont systemFontOfSize:[UIFont systemFontSize]];

  CGSize const defSize = CGSizeMake(arrow.size.width + tail.size.width + left.size.width + right.size.width,
                                    tail.size.height);
  CGSize const titleSize = aTitle ? [aTitle sizeWithFont:titleFont] : defSize;
  CGSize const descriptionSize = aDescription ? [aDescription sizeWithFont:descriptionFont] : defSize;

  CGFloat const minScreenWidth = MIN([UIScreen mainScreen].applicationFrame.size.width,
                                     [UIScreen mainScreen].applicationFrame.size.height);

  CGFloat const padding = 1.;

  // Generated image size
  CGFloat const height = tail.size.height;
  CGFloat const additionalPadding = padding * 3 + arrow.size.width + left.size.width + right.size.width;
  CGFloat const width = MAX(MIN(minScreenWidth, titleSize.width + additionalPadding),
                            MIN(minScreenWidth, descriptionSize.width + additionalPadding));

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
  CGFloat const titleTextH = left.size.height / (aDescription ? 2. : 1.);
  [[UIColor whiteColor] set];
  [aTitle drawInRect:CGRectMake(left.size.width, titleTextH / (aDescription ? 8 : 4), textW, titleTextH / 2.) withFont:titleFont lineBreakMode:UILineBreakModeTailTruncation];
  if (aDescription)
  {
    [[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0] set];
    [aDescription drawInRect:CGRectMake(left.size.width, titleTextH - (titleTextH / 8), textW, titleTextH) withFont:descriptionFont lineBreakMode:UILineBreakModeTailTruncation];
  }

  // Draw Arrow image
  CGFloat const arrowPadding = (left.size.height - arrow.size.height) / 2;
  [arrow drawAtPoint:CGPointMake(width - arrow.size.width - arrowPadding, arrowPadding)];

  UIImage * theImage = UIGraphicsGetImageFromCurrentImageContext();

  UIGraphicsEndImageContext();
	// return the image
	return theImage;
}

- (void) showInView:(UIView *)view atPoint:(CGPoint)pt
{
  if (isDisplayed)
    [self hide];

  isDisplayed = YES;

  CGFloat const w = self.pinImage.bounds.size.width;
  CGFloat const h = self.pinImage.bounds.size.height;
  // Fix point
  pt.y -= h;
  self.pinImage.frame = CGRectMake(pt.x - w/2, pt.y, w, h);
  // Do not show pin if we're editing existing bookmark.
  // @TODO move pin (and probably balloon drawing) to cross-platform code
  self.pinImage.hidden = IsValid(editedBookmark);

  [view addSubview:self.pinImage];

//  m_titleView.image = [[self createPopupImageWithName:self.title andAddress:self.description] autorelease];
  CGSize const s = m_titleView.bounds.size;
  m_titleView.frame = CGRectMake(pt.x, pt.y, 1, 1);

  [view addSubview:m_titleView];

  // Animate balloon from touched point
  [UIView transitionWithView:self.pinImage duration:0.1 options:UIViewAnimationOptionCurveEaseIn
                  animations:^{ m_titleView.frame = CGRectMake(pt.x - s.width/2, pt.y - s.height, s.width, s.height);}
                  completion:nil];
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
    editedBookmark = MakeEmptyBookmarkAndCategory();
  }
}

// Overrided property setter to reload another pin image
- (void) setColor:(NSString *)newColor
{
  id old = color;
  color = [newColor retain];
  [old release];
  self.pinImage.image = [UIImage imageNamed:newColor];
}

// Overrided property setter to save default set name into the settings
- (void) setSetName:(NSString *)newName
{
  id old = setName;
  setName = [newName retain];
  [old release];
  Settings::Set(SETTINGS_LAST_BOOKMARK_SET, string([newName UTF8String]));
}

- (void) setTitle:(NSString *)newTitle
{
  id old = title;
  title = [newTitle retain];
  [old release];
  //m_titleView.image = [self createPopupImageWithName:newTitle andAddress:description];
  m_titleView.image = [self createPopupImageWithTitle:newTitle andDescription:nil];
  [m_titleView sizeToFit];
}

- (void) addOrEditBookmark
{
  Framework & f = GetFramework();
  Bookmark bm(m2::PointD(globalPosition.x, globalPosition.y),
              [title UTF8String], [color UTF8String]);
  if (description)
    bm.SetDescription([description UTF8String]);
  f.GetBmCategory(editedBookmark.first)->ReplaceBookmark(editedBookmark.second, bm);

  BookmarkCategory * cat = f.GetBmCategory(editedBookmark.first);

  // Enable category visibility if it was turned off, so user can see newly added or edited bookmark
  if (!cat->IsVisible())
    cat->SetVisible(true);

  // Save all changes
  cat->SaveToKMLFile();
}

- (void) deleteBookmark
{
  if (IsValid(editedBookmark))
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(editedBookmark.first);
    if (cat)
    {
      cat->DeleteBookmark(editedBookmark.second);
      cat->SaveToKMLFile();
    }
    // Clear!
    editedBookmark = MakeEmptyBookmarkAndCategory();
  }
}

- (void) addBookmarkToCategory:(size_t)index
{
  Framework &f = GetFramework();
  Bookmark bm(m2::PointD(globalPosition.x, globalPosition.y),
              [title UTF8String], [color UTF8String]);
  size_t newPosition = f.AddBookmark(index, bm);
  self.editedBookmark = pair <int, int> (index, newPosition);
  self.setName = [NSString stringWithUTF8String:f.GetBmCategory(index)->GetName().c_str()];
}

@end
