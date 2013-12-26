//
//  MMAdView.h
//  MMAdView
//
//  Copyright (c) 2013 Millennial Media Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MMSDK.h"

@interface MMAdView : UIView

// Set view controller to show overlay from
@property (nonatomic, assign) UIViewController *rootViewController;
@property (nonatomic) UIInterfaceOrientation overlayOrientation;

// Initializes the ad view and pass in the controller to present overlays from
- (MMAdView *)initWithFrame:(CGRect)frame apid:(NSString *)apid rootViewController:(UIViewController *)viewController;

// Request a new ad with a generic MMRequest
- (void)getAd:(MMCompletionBlock)callback;

// Request a new ad with a custom MMRequest object (pass in location, keywords, & demographic information)
- (void)getAdWithRequest:(MMRequest *)request onCompletion:(MMCompletionBlock)callback;

@end