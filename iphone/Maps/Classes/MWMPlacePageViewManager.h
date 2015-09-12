#import <Foundation/Foundation.h>

#include "map/user_mark.hpp"

@class MWMPlacePageEntity, MWMPlacePageNavigationBar;
@protocol MWMPlacePageViewManagerProtocol;

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;
@property (nonatomic, readonly) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePageNavigationBar * iPhoneNavigationBar;
@property (nonatomic) CGFloat topBound;
@property (nonatomic, readonly) BOOL isDirectionViewShown;

- (instancetype)initWithViewController:(UIViewController *)viewController delegate:(id<MWMPlacePageViewManagerProtocol>)delegate;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;
- (void)refreshPlacePage;
- (void)dismissPlacePage;
- (void)buildRoute;
- (void)share;
- (void)addBookmark;
- (void)removeBookmark;
- (void)apiBack;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)reloadBookmark;
- (void)dragPlacePage:(CGPoint)point;
- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type;
- (void)hideDirectionView;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;

- (instancetype)init __attribute__((unavailable("call initWithViewController: instead")));

@end
