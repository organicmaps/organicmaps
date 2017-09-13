#import "MWMAPIBar.h"
#import "MWMAPIBarView.h"
#import "MWMCommon.h"
#import "Statistics.h"

#include "Framework.h"

static NSString * const kKeyPath = @"subviews";

@interface MWMAPIBar ()

@property (nonatomic) IBOutlet MWMAPIBarView * rootView;
@property (weak, nonatomic) IBOutlet UIImageView * backArrow;
@property (weak, nonatomic) IBOutlet UILabel * backLabel;
@property (weak, nonatomic) IBOutlet UILabel * timeLabel;

@property (nonatomic) NSDateFormatter * timeFormatter;
@property (nonatomic) NSTimer * timer;

@property (weak, nonatomic) UIViewController * controller;

@end

@implementation MWMAPIBar

- (nullable instancetype)initWithController:(nonnull UIViewController *)controller
{
  self = [super init];
  if (self)
  {
    self.controller = controller;
    [NSBundle.mainBundle loadNibNamed:@"MWMAPIBarView" owner:self options:nil];

    self.timeFormatter = [[NSDateFormatter alloc] init];
    self.timeFormatter.dateStyle = NSDateFormatterNoStyle;
    self.timeFormatter.timeStyle = NSDateFormatterShortStyle;
  }
  return self;
}

- (void)dealloc
{
  self.isVisible = NO;
}

- (void)timerUpdate
{
  self.timeLabel.text = [self.timeFormatter stringFromDate:[NSDate date]];
}

#pragma mark - Actions

- (IBAction)back
{
  [Statistics logEvent:kStatEventName(kStatAPI, kStatBack)];
  Framework & f = GetFramework();
  f.DeactivateMapSelection(true);
  UserMarkNotificationGuard guard(f.GetBookmarkManager(), UserMarkType::API_MARK);
  guard.m_controller.Clear();
  self.isVisible = NO;
  NSURL * url = [NSURL URLWithString:@(f.GetApiDataHolder().GetGlobalBackUrl().c_str())];
  [UIApplication.sharedApplication openURL:url];
}

#pragma mark - Properties

@synthesize isVisible = _isVisible;

- (BOOL)isVisible
{
  if (isIOS8)
    return _isVisible;
  return NO;
}

- (void)setIsVisible:(BOOL)isVisible
{
  // Status bar in iOS 9 already provides back button if the app has been launched from another app.
  // For iOS version less than 9 we just try to mimic the default iOS 9 status bar.
  if (!isIOS8)
    return;
  if (_isVisible == isVisible)
    return;
  _isVisible = isVisible;
  UIViewController * controller = self.controller;
  if (isVisible)
  {
    self.backLabel.text = [NSString
        stringWithFormat:L(@"back_to"), @(GetFramework().GetApiDataHolder().GetAppTitle().c_str())];
    [controller.view addSubview:self.rootView];
    [controller.view addObserver:self
                      forKeyPath:kKeyPath
                         options:NSKeyValueObservingOptionNew
                         context:nullptr];
    [self timerUpdate];
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                  target:self
                                                selector:@selector(timerUpdate)
                                                userInfo:nil
                                                 repeats:YES];
  }
  else
  {
    [controller.view removeObserver:self forKeyPath:kKeyPath];
    [self.rootView removeFromSuperview];
    [self.timer invalidate];
  }
  [controller setNeedsStatusBarAppearanceUpdate];
}

@end
