#import "MWMMapViewControlsManager.h"

#include "Framework.h"

@class MWMPlacePageEntity, MWMPlacePageNavigationBar, MWMViewController;

@interface MWMPlacePageViewManager : NSObject

@property(weak, nonatomic, readonly) MWMViewController * ownerViewController;
@property(nonatomic, readonly) MWMPlacePageEntity * entity;
@property(nonatomic) MWMPlacePageNavigationBar * iPhoneNavigationBar;
@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;
@property(nonatomic, readonly) BOOL isDirectionViewShown;

- (instancetype)initWithViewController:(MWMViewController *)viewController;
- (void)showPlacePage:(place_page::Info const &)info;
- (void)refreshPlacePage;
- (void)mwm_refreshUI;
- (BOOL)hasPlacePage;
- (void)dismissPlacePage;
- (void)hidePlacePage;
- (void)routeFrom;
- (void)routeTo;
- (void)share;
- (void)editPlace;
- (void)addBusiness;
- (void)addPlace;
- (void)addBookmark;
- (void)removeBookmark;
- (void)book:(BOOL)isDecription;
- (void)call;
- (void)apiBack;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)reloadBookmark;
- (void)dragPlacePage:(CGRect)frame;
- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type;
- (void)hideDirectionView;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;
- (void)changeHeight:(CGFloat)height;

- (instancetype)init __attribute__((unavailable("call initWithViewController: instead")));

@end
