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

#import "FIRCrashlytics.h"
#import "FirebaseCrashlytics.h"
#import "FIRExceptionModel.h"
#import "FIRStackFrame.h"

FOUNDATION_EXPORT double FirebaseCrashlyticsVersionNumber;
FOUNDATION_EXPORT const unsigned char FirebaseCrashlyticsVersionString[];

