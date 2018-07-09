#import "MWMSearchNoResultsAlert.h"
#import "MWMCommon.h"
#import "MWMSearch.h"
#import "Statistics.h"

#include "Framework.h"

namespace
{
NSString * const kStatisticsEvent = @"Search No Results Alert";
}

@interface MWMSearchNoResultsAlert ()

@property(nonatomic) IBOutletCollection(UIView) NSArray * resetFilterViews;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * twoButtonsOffset;

@end

@implementation MWMSearchNoResultsAlert

+ (instancetype)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMSearchNoResultsAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  return alert;
}

- (void)update
{
  [self layoutIfNeeded];
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     UILayoutPriority priority = UILayoutPriorityDefaultHigh;
                     CGFloat alpha = 0;
                     if ([MWMSearch hasFilter])
                     {
                       priority = UILayoutPriorityDefaultLow;
                       alpha = 1;
                     }
                     self.twoButtonsOffset.priority = priority;
                     for (UIView * view in self.resetFilterViews)
                       view.alpha = alpha;

                     [self setNeedsLayout];
                   }];
}

- (IBAction)expandSearchAreaTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatExpand}];
  [self close:^{
    GetFramework().Scale(Framework::SCALE_MIN, true);
  }];
}

- (IBAction)resetFiltersTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatRemove}];
  [self close:^{
    [MWMSearch clearFilter];
  }];
}

- (IBAction)cancelTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatCancel}];
  [self close:nil];
}

@end
