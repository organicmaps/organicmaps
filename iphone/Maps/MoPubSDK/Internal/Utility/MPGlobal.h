//
//  MPGlobal.h
//  MoPub
//
//  Created by Andrew He on 5/5/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CJSONDeserializer.h"

#ifndef MP_ANIMATED
#define MP_ANIMATED YES
#endif

UIInterfaceOrientation MPInterfaceOrientation(void);
UIWindow *MPKeyWindow(void);
CGFloat MPStatusBarHeight(void);
CGRect MPApplicationFrame(void);
CGRect MPScreenBounds(void);
CGFloat MPDeviceScaleFactor(void);
NSDictionary *MPDictionaryFromQueryString(NSString *query);
BOOL MPViewIsVisible(UIView *view);

////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Availability constants.
 */

#define MP_IOS_2_0  20000
#define MP_IOS_2_1  20100
#define MP_IOS_2_2  20200
#define MP_IOS_3_0  30000
#define MP_IOS_3_1  30100
#define MP_IOS_3_2  30200
#define MP_IOS_4_0  40000
#define MP_IOS_4_1  40100
#define MP_IOS_4_2  40200
#define MP_IOS_4_3  40300
#define MP_IOS_5_0  50000
#define MP_IOS_5_1  50100
#define MP_IOS_6_0  60000
#define MP_IOS_7_0  70000

////////////////////////////////////////////////////////////////////////////////////////////////////

enum {
    MPInterstitialCloseButtonStyleAlwaysVisible,
    MPInterstitialCloseButtonStyleAlwaysHidden,
    MPInterstitialCloseButtonStyleAdControlled
};
typedef NSUInteger MPInterstitialCloseButtonStyle;

enum {
    MPInterstitialOrientationTypePortrait,
    MPInterstitialOrientationTypeLandscape,
    MPInterstitialOrientationTypeAll
};
typedef NSUInteger MPInterstitialOrientationType;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface CJSONDeserializer (MPAdditions)

+ (CJSONDeserializer *)deserializerWithNullObject:(id)obj;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface NSString (MPAdditions)

/*
 * Returns string with reserved/unsafe characters encoded.
 */
- (NSString *)URLEncodedString;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface UIDevice (MPAdditions)

- (NSString *)hardwareDeviceName;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////
// Optional Class Forward Def Protocols
////////////////////////////////////////////////////////////////////////////////////////////////////

@class MPAdConfiguration, CLLocation;

@protocol MPAdAlertManagerProtocol <NSObject>

@property (nonatomic, retain) MPAdConfiguration *adConfiguration;
@property (nonatomic, copy) NSString *adUnitId;
@property (nonatomic, copy) CLLocation *location;
@property (nonatomic, assign) UIView *targetAdView;
@property (nonatomic, assign) id delegate;

- (void)beginMonitoringAlerts;
- (void)processAdAlertOnce;

@end
