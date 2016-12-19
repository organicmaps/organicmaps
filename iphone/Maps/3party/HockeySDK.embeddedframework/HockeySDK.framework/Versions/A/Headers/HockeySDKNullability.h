//
//  HockeyNullability.h
//  HockeySDK
//
//  Created by Andreas Linde on 12/06/15.
//
//

#ifndef HockeySDK_HockeyNullability_h
#define HockeySDK_HockeyNullability_h

// Define nullability fallback for backwards compatibility
#if !__has_feature(nullability)
#define NS_ASSUME_NONNULL_BEGIN
#define NS_ASSUME_NONNULL_END
#define nullable
#define nonnull
#define null_unspecified
#define null_resettable
#define __nullable
#define __nonnull
#define __null_unspecified
#endif

// Fallback for convenience syntax which might not be available in older SDKs
#ifndef NS_ASSUME_NONNULL_BEGIN
#define NS_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#endif
#ifndef NS_ASSUME_NONNULL_END
#define NS_ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#endif

#endif /* HockeySDK_HockeyNullability_h */
