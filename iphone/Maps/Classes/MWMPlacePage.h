//
//  MWMPlacePageView.h
//  Maps
//
//  Created by v.mikhaylenko on 18.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMBasePlacePageView, MWMPlacePageViewManager, MWMPlacePageActionBar;

@interface MWMPlacePage : NSObject

@property (weak, nonatomic) IBOutlet MWMBasePlacePageView * basePlacePageView;
@property (weak, nonatomic) IBOutlet UIView * extendedPlacePageView;
@property (weak, nonatomic) IBOutlet UIImageView * anchorImageView;
@property (weak, nonatomic) IBOutlet UIPanGestureRecognizer * panRecognizer;
@property (weak, nonatomic, readonly) MWMPlacePageViewManager * manager;
@property (nonatomic) MWMPlacePageActionBar * actionBar;
@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat parentViewHeight;
@property (nonatomic) CGFloat keyboardHeight;

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager;
- (void)show;
- (void)dismiss;
- (void)configure;

#pragma mark - Actions
- (void)addBookmark;
- (void)removeBookmark;
- (void)changeBookmarkColor;
- (void)changeBookmarkCategory;
- (void)changeBookmarkDescription;
- (void)share;
- (void)route;
- (void)stopBuildingRoute;
- (void)reloadBookmark;
- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight;
- (void)willFinishEditingBookmarkTitle:(NSString *)title;
- (void)addPlacePageShadowToView:(UIView *)view;

- (IBAction)didTap:(UITapGestureRecognizer *)sender;

- (void)setDirectionArrowTransform:(CGAffineTransform)transform;
- (void)setDistance:(NSString *)distance;
- (void)updateMyPositionStatus:(NSString *)status;

- (instancetype)init __attribute__((unavailable("init is unavailable, call initWithManager: instead")));

@end
