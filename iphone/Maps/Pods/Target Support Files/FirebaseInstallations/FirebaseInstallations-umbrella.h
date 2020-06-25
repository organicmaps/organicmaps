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

#import "FirebaseInstallations.h"
#import "FIRInstallations.h"
#import "FIRInstallationsAuthTokenResult.h"
#import "FIRInstallationsErrors.h"
#import "FIRInstallationsVersion.h"

FOUNDATION_EXPORT double FirebaseInstallationsVersionNumber;
FOUNDATION_EXPORT const unsigned char FirebaseInstallationsVersionString[];

