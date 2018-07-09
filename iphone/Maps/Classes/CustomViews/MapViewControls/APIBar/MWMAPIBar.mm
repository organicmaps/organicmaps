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
  auto & bmManager = f.GetBookmarkManager();
  bmManager.GetEditSession().ClearGroup(UserMark::Type::API);
  NSURL * url = [NSURL URLWithString:@(f.GetApiDataHolder().GetGlobalBackUrl().c_str())];
  [UIApplication.sharedApplication openURL:url];
}

@end
