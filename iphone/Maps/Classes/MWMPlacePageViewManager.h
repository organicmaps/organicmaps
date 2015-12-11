#include "Framework.h"

@class MWMPlacePageEntity, MWMPlacePageNavigationBar;
@protocol MWMPlacePageViewManagerProtocol;

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;
@property (nonatomic, readonly) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePageNavigationBar * iPhoneNavigationBar;
@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic, readonly) BOOL isDirectionViewShown;
@property (nonatomic, readonly) location::EMyPositionMode myPositionMode;

- (instancetype)initWithViewController:(UIViewController *)viewController
                              delegate:(id<MWMPlacePageViewManagerProtocol>)delegate;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;
- (void)refreshPlacePage;
- (void)dismissPlacePage;
- (void)hidePlacePage;
- (void)buildRoute;
- (void)routeFrom;
- (void)routeTo;
- (void)share;
- (void)addBookmark;
- (void)removeBookmark;
- (void)apiBack;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)reloadBookmark;
- (void)changeBookmarkCategory:(BookmarkAndCategory)bac;
- (void)dragPlacePage:(CGRect)frame;
- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type;
- (void)hideDirectionView;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;
- (void)changeHeight:(CGFloat)height;

- (instancetype)init __attribute__((unavailable("call initWithViewController: instead")));

@end
