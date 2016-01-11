#import "MWMSearchTextField.h"
#import "MWMSearchView.h"

typedef NS_ENUM(NSUInteger, MWMSearchManagerState)
{
  MWMSearchManagerStateHidden,
  MWMSearchManagerStateDefault,
  MWMSearchManagerStateTableSearch,
  MWMSearchManagerStateMapSearch
};

@protocol MWMSearchManagerProtocol <NSObject>

- (void)searchViewDidEnterState:(MWMSearchManagerState)state;
- (void)actionDownloadMaps;

@end

@protocol MWMRoutingProtocol;

@interface MWMSearchManager : NSObject

@property (weak, nonatomic) id <MWMSearchManagerProtocol, MWMRoutingProtocol> delegate;
@property (weak, nonatomic) IBOutlet MWMSearchTextField * searchTextField;

@property (nonatomic) MWMSearchManagerState state;

@property (nonnull, nonatomic, readonly) UIView * view;

- (nullable instancetype)init __attribute__((unavailable("init is not available")));
- (nullable instancetype)initWithParentView:(nonnull UIView *)view
                                   delegate:(nonnull id<MWMSearchManagerProtocol, MWMSearchViewProtocol, MWMRoutingProtocol>)delegate;

- (void)refresh;

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(nonnull id<UIViewControllerTransitionCoordinator>)coordinator;

@end
