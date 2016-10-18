#import "MWMMapViewControlsManager.h"

#include "Framework.h"

@class MWMPlacePageEntity, MWMViewController;

@protocol MWMActionBarProtocol<NSObject>

- (void)routeFrom;
- (void)routeTo;

- (void)share;

- (void)addBookmark;
- (void)removeBookmark;

- (void)call;
- (void)book:(BOOL)isDecription;

- (void)apiBack;

@end

@protocol MWMPlacePageButtonsProtocol<NSObject>

- (void)editPlace;
- (void)addPlace;
- (void)addBusiness;
- (void)book:(BOOL)isDescription;
- (void)editBookmark;

@end

struct FeatureID;

@protocol MWMFeatureHolder<NSObject>

- (FeatureID const &)featureId;

@end

@protocol MWMPlacePageProtocol<MWMActionBarProtocol, MWMPlacePageButtonsProtocol, MWMFeatureHolder>

@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;

- (void)showPlacePage:(place_page::Info const &)info;
- (void)mwm_refreshUI;
- (void)dismissPlacePage;
- (void)hidePlacePage;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;

@end
