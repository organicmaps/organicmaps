#import "BalloonView.h"
#import <QuartzCore/CALayer.h>

@implementation BalloonView

@synthesize title;
@synthesize description;
@synthesize isDisplayed;

- (void) setGlobalPos:(m2::PointD const &)pt
{
  m_globalPos = pt;
}

- (m2::PointD) getGlobalPos
{
  return m_globalPos;
}

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
{
  if ((self = [super init]))
  {
    isDisplayed = NO;
    m_target = target;
    m_selector = selector;
  }
  return self;
}

- (void) dealloc
{
  [m_titleView release];
  [m_pinView release];
  [super dealloc];
}

- (void) showButtonsInView:(UIView *)view atPoint:(CGPoint)pt
{
  m_titleView = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"BookmarkTitle"];
  m_titleView.textLabel.text = title;
  m_titleView.textLabel.textColor = [UIColor whiteColor];
  m_titleView.detailTextLabel.text = description;
  m_titleView.detailTextLabel.textColor = [UIColor whiteColor];
  m_titleView.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
  m_titleView.backgroundColor = [UIColor blackColor];
  m_titleView.layer.cornerRadius = 5;
  m_titleView.alpha = 0.8;
  m_titleView.textLabel.backgroundColor = [UIColor clearColor];
  m_titleView.detailTextLabel.backgroundColor = [UIColor clearColor];
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

  m_pinView = [[UIImageView alloc ]initWithImage:[UIImage imageNamed:@"marker"]];
  CGFloat const w = m_pinView.bounds.size.width;
  CGFloat const h = m_pinView.bounds.size.height;
  m_pinView.frame = CGRectMake(pt.x - w/2, 0 - h, w, h);

  [view addSubview:m_pinView];

  // Animate pin to the touched point
  [UIView transitionWithView:m_pinView duration:0.1 options:UIViewAnimationOptionCurveEaseIn
                  animations:^{ m_pinView.frame = CGRectMake(pt.x - w/2, pt.y - h, w, h); }
                  completion:^(BOOL){
                    [self showButtonsInView:view atPoint:CGPointMake(pt.x, pt.y - h)];
                  }];
}

- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt
{
  if (isDisplayed)
  {
    CGFloat const w1 = m_pinView.bounds.size.width;
    CGFloat const h1 = m_pinView.bounds.size.height;
    m_pinView.frame = CGRectMake(pt.x - w1/2, pt.y - h1, w1, h1);

    CGFloat const w2 = m_titleView.bounds.size.width;
    CGFloat const h2 = m_titleView.bounds.size.height;
    m_titleView.frame = CGRectMake(pt.x - w2/2, pt.y - h1 - h2, w2, h2);
  }
}

- (void) hide
{
  [m_titleView removeFromSuperview];
  [m_titleView release];
  m_titleView = nil;

  [m_pinView removeFromSuperview];
  [m_pinView release];
  m_pinView = nil;

  isDisplayed = NO;
}

@end
