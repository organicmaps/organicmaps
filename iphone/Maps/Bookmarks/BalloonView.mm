#import "BalloonView.h"
#import <QuartzCore/CALayer.h>


@implementation BalloonView

@synthesize globalPosition;
@synthesize title;
@synthesize description;
@synthesize pinImage;
@synthesize color;
@synthesize setName;
@synthesize isDisplayed;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
{
  if ((self = [super init]))
  {
    // Default bookmark pin color
    color = [[NSString alloc] initWithString:@"purple"];
    setName = [[NSString alloc] initWithString:NSLocalizedString(@"My Places", @"Default bookmarks set name")];
    pinImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:self.color]];
    m_titleView = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"BookmarkTitle"];
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
  [super dealloc];
}

- (void) showButtonsInView:(UIView *)view atPoint:(CGPoint)pt
{
  m_titleView.textLabel.text = self.title;
  m_titleView.textLabel.textColor = [UIColor whiteColor];
  m_titleView.detailTextLabel.text = self.description;
  m_titleView.detailTextLabel.textColor = [UIColor whiteColor];
  m_titleView.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
  m_titleView.backgroundColor = [UIColor blackColor];
  m_titleView.layer.cornerRadius = 10;
//  m_titleView.alpha = 0.8;
//  m_titleView.textLabel.backgroundColor = [UIColor clearColor];
//  m_titleView.detailTextLabel.backgroundColor = [UIColor clearColor];
  m_titleView.textLabel.textColor = [UIColor whiteColor];
  m_titleView.detailTextLabel.textColor = [UIColor whiteColor];
  CGFloat const w = m_titleView.bounds.size.width / 3 * 2;
  CGFloat const h = m_titleView.bounds.size.height;
  m_titleView.frame = CGRectMake(pt.x - w/2, pt.y - h, w, h);

  m_titleView.userInteractionEnabled = YES;
  UITapGestureRecognizer * recognizer = [[UITapGestureRecognizer alloc]
                                         initWithTarget:m_target action:m_selector];
  recognizer.numberOfTapsRequired = 1;
  recognizer.numberOfTouchesRequired = 1;
  recognizer.delaysTouchesBegan = YES;
  [m_titleView addGestureRecognizer:recognizer];
  [recognizer release];

  [view addSubview:m_titleView];
}

- (void) showInView:(UIView *)view atPoint:(CGPoint)pt
{
  if (isDisplayed)
  {
    NSLog(@"Already displaying the BalloonView");
    return;
  }
  isDisplayed = YES;

  CGFloat const w = self.pinImage.bounds.size.width;
  CGFloat const h = self.pinImage.bounds.size.height;
  self.pinImage.frame = CGRectMake(pt.x - w/2, 0 - h, w, h);

  [view addSubview:self.pinImage];

  // Animate pin to the touched point
  [UIView transitionWithView:self.pinImage duration:0.1 options:UIViewAnimationOptionCurveEaseIn
                  animations:^{ self.pinImage.frame = CGRectMake(pt.x - w/2, pt.y - h, w, h); }
                  completion:^(BOOL){
                    [self showButtonsInView:view atPoint:CGPointMake(pt.x, pt.y - h)];
                  }];
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
  [m_titleView removeFromSuperview];
  [self.pinImage removeFromSuperview];
  isDisplayed = NO;
}

// Overrided property setter to update address and displayed information
- (void) setGlobalPosition:(CGPoint)pos
{
  globalPosition = pos;
  GetFramework().GetAddressInfo(m2::PointD(pos.x, pos.y), m_addressInfo);
  if (m_addressInfo.m_name.empty())
    self.title = NSLocalizedString(@"Dropped Pin", @"Unknown Dropped Pin title, when name can't be determined");
  else
    self.title = [NSString stringWithUTF8String:m_addressInfo.m_name.c_str()];
  self.description = [NSString stringWithUTF8String:m_addressInfo.FormatAddress().c_str()];
}

// Overrided property setter to reload another pin image
- (void) setColor:(NSString *)newColor
{
  color = newColor;
  self.pinImage.image = [UIImage imageNamed:newColor];
}
@end
