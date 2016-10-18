#import "MWMMapViewControlsManager.h"

#include "Framework.h"

#include "MWMPlacePageProtocol.h"

@class MWMPlacePageEntity, MWMPlacePageNavigationBar, MWMViewController;

@interface MWMPlacePageViewManager : NSObject<MWMPlacePageProtocol>

@property(weak, nonatomic, readonly) MWMViewController * ownerViewController;
@property(nonatomic, readonly) MWMPlacePageEntity * entity;
@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;

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
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;
- (void)changeHeight:(CGFloat)height;

@end
