//
//  FacebookNativeCustomEvent.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#if __has_include(<MoPub/MoPub.h>)
    #import <MoPub/MoPub.h>
#else
    #import "MPNativeCustomEvent.h"
#endif

/*
 * Certified with Facebook Audience Network 4.26.1
 */
@interface FacebookNativeCustomEvent : MPNativeCustomEvent


/**
 * Toggle FB video ads on/off. If it is enabled, it means you are open yourself to video inventory.
 * If it is not enabled, it is gauranteed you won't get video ads.
 *
 * IMPORTANT: If you choose to use this method, be sure to call it before making any ad requests,
 * and avoid calling it more than once.
 */

+ (void)setVideoEnabled:(BOOL)enabled;

@end
