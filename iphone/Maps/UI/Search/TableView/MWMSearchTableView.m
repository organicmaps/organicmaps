#import "MWMSearchTableView.h"
#import "MWMSearchNoResults.h"

@interface MWMSearchTableView ()

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * noResultsBottomOffset;

@property(weak, nonatomic) IBOutlet UIView * noResultsContainer;
@property(weak, nonatomic) IBOutlet UIView * noResultsWrapper;
@property(nonatomic) MWMSearchNoResults * noResultsView;

@end

@implementation MWMSearchTableView

- (void)awakeFromNib
{
  [super awakeFromNib];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)hideNoResultsView:(BOOL)hide
{
  if (hide)
  {
    self.noResultsContainer.hidden = YES;
    [self.noResultsView removeFromSuperview];
  }
  else
  {
    self.noResultsContainer.hidden = NO;
    [self.noResultsWrapper addSubview:self.noResultsView];
  }
}

- (MWMSearchNoResults *)noResultsView
{
  if (!_noResultsView)
  {
    _noResultsView = [MWMSearchNoResults viewWithImage:nil
                                                 title:L(@"search_not_found")
                                                  text:L(@"search_not_found_query")];
  }
  return _noResultsView;
}

@end
