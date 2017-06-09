//
//  MTRGNativePromoCard.h
//  myTargetSDK 4.6.15
//
//  Created by Andrey Seredkin on 18.10.16.
//  Copyright © 2016 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MyTargetSDK/MTRGImageData.h>

@interface MTRGNativePromoCard : NSObject

@property(nonatomic, copy) NSString *title;
@property(nonatomic, copy) NSString *descriptionText;
@property(nonatomic, copy) NSString *ctaText;
@property(nonatomic) MTRGImageData *image;

@end
