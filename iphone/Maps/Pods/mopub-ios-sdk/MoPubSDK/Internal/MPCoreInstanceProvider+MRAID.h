//
//  MPCoreInstanceProvider+MRAID.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCoreInstanceProvider.h"

@interface MPCoreInstanceProvider (MRAID)

/**
 Returns @c YES if MRAID.js loaded correctly, @c NO otherwise.
 */
@property (nonatomic, readonly) BOOL isMraidJavascriptAvailable;

/**
 Returns an @c NSString containing the MRAID.js javascript to be loaded into webviews created for MRAID ads. If MRAID.js
 is missing, this property returns @c nil instead.
 */
@property (nonatomic, readonly, copy) NSString * mraidJavascript;

@end
