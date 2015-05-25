//
//  PFNullability.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#ifndef Parse_PFNullability_h
#define Parse_PFNullability_h

///--------------------------------------
/// @name Nullability Annotation Support
///--------------------------------------

#if __has_feature(nullability)
#  define PF_NONNULL nonnull
#  define PF_NONNULL_S __nonnull
#  define PF_NULLABLE nullable
#  define PF_NULLABLE_S __nullable
#  define PF_NULLABLE_PROPERTY nullable,
#else
#  define PF_NONNULL
#  define PF_NONNULL_S
#  define PF_NULLABLE
#  define PF_NULLABLE_S
#  define PF_NULLABLE_PROPERTY
#endif

#if __has_feature(assume_nonnull)
#  ifdef NS_ASSUME_NONNULL_BEGIN
#    define PF_ASSUME_NONNULL_BEGIN NS_ASSUME_NONNULL_BEGIN
#  else
#    define PF_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#  endif
#  ifdef NS_ASSUME_NONNULL_END
#    define PF_ASSUME_NONNULL_END NS_ASSUME_NONNULL_END
#  else
#    define PF_ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#  endif
#else
#  define PF_ASSUME_NONNULL_BEGIN
#  define PF_ASSUME_NONNULL_END
#endif

#endif
