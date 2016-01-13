#import "MWMSearchTabbedViewProtocol.h"
#import "MWMSearchTabButtonsView.h"
#import "ViewController.h"

@interface MWMSearchTabbedViewController : ViewController

@property (copy, nonatomic) NSArray * tabButtons;
@property (weak, nonatomic) NSLayoutConstraint * scrollIndicatorOffset;
@property (weak, nonatomic) UIView * scrollIndicator;
@property (weak, nonatomic) id<MWMSearchTabbedViewProtocol> delegate;

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender;
- (void)resetSelectedTab;

@end
