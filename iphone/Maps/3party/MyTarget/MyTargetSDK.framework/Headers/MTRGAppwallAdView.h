//
//  MTRGAppwallAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 16.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativeAppwallBanner.h>

@protocol MTRGAppwallAdViewDelegate <NSObject>

- (void)appwallAdViewOnClickWithBanner:(MTRGNativeAppwallBanner *)banner;

- (void)appwallAdViewOnShowWithBanner:(MTRGNativeAppwallBanner *)banner;

@end

@interface MTRGAppwallAdView : UIView
@property(nonatomic, weak) id <MTRGAppwallAdViewDelegate> delegate;

- (instancetype)initWithBanners:(NSArray *)banners;

@end
