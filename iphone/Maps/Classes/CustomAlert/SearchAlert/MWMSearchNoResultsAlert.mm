#import "MWMSearchNoResultsAlert.h"
#import "MWMSearch.h"

#include <CoreApi/Framework.h>

@interface MWMSearchNoResultsAlert ()

@property(nonatomic) IBOutletCollection(UIView) NSArray * resetFilterViews;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * twoButtonsOffset;

@end

@implementation MWMSearchNoResultsAlert

+ (instancetype)alert
{
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
  [self close:^{
    GetFramework().Scale(Framework::SCALE_MIN, true);
  }];
}

- (IBAction)resetFiltersTap
{
  [self close:^{
    [MWMSearch clearFilter];
  }];
}

- (IBAction)cancelTap
{
  [self close:nil];
}

@end
