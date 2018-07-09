#import "MWMSearchChangeModeView.h"
#import "MWMButton.h"
#import "MWMSearch.h"
#import "UIButton+RuntimeAttributes.h"

extern NSString * const kSearchStateKey;

@interface MWMSearchChangeModeView ()<MWMSearchObserver>

@property(weak, nonatomic) IBOutlet UIButton * changeModeButton;

@property(weak, nonatomic) IBOutlet UIButton * filterButton;
@property(weak, nonatomic) IBOutlet MWMButton * cancelFilterButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * filterButtoniPadX;
@property(weak, nonatomic) IBOutlet UIView * changeModeBackground;

@end

@implementation MWMSearchChangeModeView

- (void)awakeFromNib
{
  [super awakeFromNib];
  [MWMSearch addObserver:self];
  self.changeModeButton.titleLabel.textAlignment = NSTextAlignmentNatural;
  self.filterButton.titleLabel.textAlignment = NSTextAlignmentNatural;
  self.filterButtoniPadX.priority = IPAD ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

- (void)updateForState:(MWMSearchManagerState)state
{
  UIButton * changeModeButton = self.changeModeButton;
  if (IPAD)
  {
    changeModeButton.hidden = YES;
    return;
  }
  switch (state)
  {
  case MWMSearchManagerStateTableSearch:
    self.backgroundColor = [UIColor pressBackground];
    [changeModeButton setTitle:L(@"search_show_on_map") forState:UIControlStateNormal];
    break;
  case MWMSearchManagerStateMapSearch:
    self.backgroundColor = [UIColor white];
    [changeModeButton setTitle:L(@"search_in_table") forState:UIControlStateNormal];
    break;
  default: break;
  }
}

- (void)updateFilterButtons:(BOOL)isFilterResults
{
  BOOL const hasFilter = [MWMSearch hasFilter];
  BOOL const hide = !(isFilterResults || hasFilter);
  self.filterButton.hidden = hide;
  self.cancelFilterButton.hidden = hide;
  if (hide)
    return;
  if (hasFilter)
  {
    [self.filterButton setBackgroundColorName:@"linkBlue"];
    [self.filterButton setBackgroundHighlightedColorName:@"linkBlueHighlighted"];
    [self.filterButton setTitleColor:[UIColor white] forState:UIControlStateNormal];
    [self.cancelFilterButton setImage:[UIImage imageNamed:@"ic_clear_filters"]
                             forState:UIControlStateNormal];
    self.cancelFilterButton.coloring = MWMButtonColoringWhite;
    [self bringSubviewToFront:self.cancelFilterButton];
  }
  else
  {
    [self.filterButton setBackgroundColorName:@"clearColor"];
    [self.filterButton setBackgroundHighlightedColorName:@"clearColor"];
    [self.filterButton setTitleColor:[UIColor linkBlue] forState:UIControlStateNormal];
    [self.cancelFilterButton setImage:[UIImage imageNamed:@"ic_filter"]
                             forState:UIControlStateNormal];
    self.cancelFilterButton.coloring = MWMButtonColoringBlue;
    [self sendSubviewToBack:self.cancelFilterButton];
  }
  [self sendSubviewToBack:self.changeModeBackground];
}

#pragma mark - MWMSearchObserver

- (void)onSearchStarted { [self updateFilterButtons:[MWMSearch isHotelResults]]; }
- (void)onSearchCompleted { [self updateFilterButtons:[MWMSearch isHotelResults]]; }

@end
