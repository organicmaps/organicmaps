//
//  MTRGTypes.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 28.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

//Тип навигации для банера
typedef enum{
    MTRGNavigationTypeWeb = 1,
    MTRGNavigationTypeStore,
} MTRGNavigationType;

//Форматы нативных банеров
extern NSString *const kMTRGAdBannerFormatNativeBanner;
extern NSString *const kMTRGAdBannerFormatNativeTeaser;
extern NSString *const kMTRGAdBannerFormatNativePromo;
