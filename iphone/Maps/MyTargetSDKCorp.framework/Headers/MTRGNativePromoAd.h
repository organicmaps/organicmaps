//
//  MTRGNativePromoAd.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 10.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <MyTargetSDKCorp/MTRGNativePromoBanner.h>
#import <MyTargetSDKCorp/MTRGNativeBaseAd.h>

@class MTRGNativePromoAd;

@protocol MTRGNativePromoAdDelegate <NSObject>

-(void)onLoadWithNativePromoBanner:(MTRGNativePromoBanner *)promoBanner promoAd:(MTRGNativePromoAd *)promoAd;
-(void)onNoAdWithReason:(NSString *)reason promoAd:(MTRGNativePromoAd *)promoAd;

@optional

-(void)onAdClickWithNativePromoAd:(MTRGNativePromoAd *)promoAd;

@end

@interface MTRGNativePromoAd : MTRGNativeBaseAd

-(instancetype) initWithSlotId:(NSString*)slotId;

//Делегат
@property (weak, nonatomic) id<MTRGNativePromoAdDelegate> delegate;
//Загрузить картинку в UIImageView
-(void) loadImageToView:(UIImageView*) imageView;
//Загрузить Иконку в UIImageView
-(void) loadIconToView:(UIImageView*) imageView;

@property (strong, nonatomic, readonly) MTRGNativePromoBanner * banner;

@end
