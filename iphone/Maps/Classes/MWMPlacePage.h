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
@property (weak, nonatomic) IBOutlet UIImageView *anchorImageView;
@property (weak, nonatomic, readonly) MWMPlacePageViewManager * manager;
@property (nonatomic) MWMPlacePageActionBar * actionBar;

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager;
- (void)show;
- (void)dismiss;
- (void)configure;
- (void)addBookmark;

- (instancetype)init __attribute__((unavailable("init is unavailable, call initWithManager: instead")));

@end
