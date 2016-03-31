//
//  MTRGNativeImageAd.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 10.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <MyTargetSDKCorp/MTRGNativeImageBanner.h>
#import <MyTargetSDKCorp/MTRGNativeBaseAd.h>

@class MTRGNativeImageAd;

@protocol MTRGNativeImageAdDelegate <NSObject>

-(void)onLoadWithNativeImageBanner:(MTRGNativeImageBanner *)imageBanner imageAd:(MTRGNativeImageAd *)imageAd;
-(void)onNoAdWithReason:(NSString *)reason imageAd:(MTRGNativeImageAd *)imageAd;

@optional

-(void)onAdClickWithNativeImageAd:(MTRGNativeImageAd *)imageAd;

@end


@interface MTRGNativeImageAd : MTRGNativeBaseAd

//Делегат
@property (weak, nonatomic) id<MTRGNativeImageAdDelegate> delegate;

//Загрузить картинку в UIImageView
-(void) loadImageToView:(UIImageView*) imageView;

-(instancetype) initWithSlotId:(NSString*)slotId;

@property (strong, nonatomic, readonly) MTRGNativeImageBanner * banner;

@end
