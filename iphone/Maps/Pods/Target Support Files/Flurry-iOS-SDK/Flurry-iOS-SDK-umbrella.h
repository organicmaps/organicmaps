#ifdef __OBJC__
#import <UIKit/UIKit.h>
#else
#ifndef FOUNDATION_EXPORT
#if defined(__cplusplus)
#define FOUNDATION_EXPORT extern "C"
#else
#define FOUNDATION_EXPORT extern
#endif
#endif
#endif

#import "Flurry.h"
#import "FlurrySessionBuilder.h"
#import "FlurryConsent.h"
#import "FlurryUserProperties.h"
#import "FlurryCCPA.h"
#import "FlurrySKAdNetwork.h"

FOUNDATION_EXPORT double Flurry_iOS_SDKVersionNumber;
FOUNDATION_EXPORT const unsigned char Flurry_iOS_SDKVersionString[];

