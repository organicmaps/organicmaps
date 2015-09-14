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

- (void)searchViewWillEnterState:(MWMSearchManagerState)state;
- (void)searchViewDidEnterState:(MWMSearchManagerState)state;
- (void)actionDownloadMaps;

@end

@interface MWMSearchManager : NSObject

@property (nonnull, weak, nonatomic) id <MWMSearchManagerProtocol> delegate;
@property (nonnull, weak, nonatomic) IBOutlet MWMSearchTextField * searchTextField;

@property (nonatomic) MWMSearchManagerState state;

@property (nonnull, nonatomic, readonly) UIView * view;

- (nullable instancetype)init __attribute__((unavailable("init is not available")));
- (nullable instancetype)initWithParentView:(nonnull UIView *)view
                                   delegate:(nonnull id<MWMSearchManagerProtocol, MWMSearchViewProtocol>)delegate;

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(nonnull id<UIViewControllerTransitionCoordinator>)coordinator;

@end
