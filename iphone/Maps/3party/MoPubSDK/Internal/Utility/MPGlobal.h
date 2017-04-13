//
//  MPGlobal.h
//  MoPub
//
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifndef MP_ANIMATED
#define MP_ANIMATED YES
#endif

UIInterfaceOrientation MPInterfaceOrientation(void);
UIWindow *MPKeyWindow(void);
CGFloat MPStatusBarHeight(void);
CGRect MPApplicationFrame(void);
CGRect MPScreenBounds(void);
CGSize MPScreenResolution(void);
CGFloat MPDeviceScaleFactor(void);
NSDictionary *MPDictionaryFromQueryString(NSString *query);
NSString *MPSHA1Digest(NSString *string);
BOOL MPViewIsVisible(UIView *view);
BOOL MPViewIntersectsParentWindowWithPercent(UIView *view, CGFloat percentVisible);
NSString *MPResourcePathForResource(NSString *resourceName);
NSArray *MPConvertStringArrayToURLArray(NSArray *strArray);
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
#define MP_IOS_8_0  80000
#define MP_IOS_9_0  90000

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

@interface NSString (MPAdditions)

/*
 * Returns string with reserved/unsafe characters encoded.
 */
- (NSString *)mp_URLEncodedString;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface UIDevice (MPAdditions)

- (NSString *)mp_hardwareDeviceName;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface UIApplication (MPAdditions)

// Correct way to hide/show the status bar on pre-ios 7.
- (void)mp_preIOS7setApplicationStatusBarHidden:(BOOL)hidden;
- (BOOL)mp_supportsOrientationMask:(UIInterfaceOrientationMask)orientationMask;
- (BOOL)mp_doesOrientation:(UIInterfaceOrientation)orientation matchOrientationMask:(UIInterfaceOrientationMask)orientationMask;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////
// Optional Class Forward Def Protocols
////////////////////////////////////////////////////////////////////////////////////////////////////

@class MPAdConfiguration, CLLocation;

@protocol MPAdAlertManagerProtocol <NSObject>

@property (nonatomic, strong) MPAdConfiguration *adConfiguration;
@property (nonatomic, copy) NSString *adUnitId;
@property (nonatomic, copy) CLLocation *location;
@property (nonatomic, weak) UIView *targetAdView;
@property (nonatomic, weak) id delegate;

- (void)beginMonitoringAlerts;
- (void)endMonitoringAlerts;
- (void)processAdAlertOnce;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////
// Small alert wrapper class to handle telephone protocol prompting
////////////////////////////////////////////////////////////////////////////////////////////////////

@class MPTelephoneConfirmationController;

typedef void (^MPTelephoneConfirmationControllerClickHandler)(NSURL *targetTelephoneURL, BOOL confirmed);

@interface MPTelephoneConfirmationController : NSObject <UIAlertViewDelegate>

- (id)initWithURL:(NSURL *)url clickHandler:(MPTelephoneConfirmationControllerClickHandler)clickHandler;
- (void)show;

@end
