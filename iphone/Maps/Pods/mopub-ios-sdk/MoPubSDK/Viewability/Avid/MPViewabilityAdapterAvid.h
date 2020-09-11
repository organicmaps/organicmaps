//
//  MPViewabilityAdapterAvid.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#if __has_include(<MoPub/MoPub.h>)
#import <MoPub/MoPub.h>
#elif __has_include(<MoPubSDKFramework/MoPub.h>)
#import <MoPubSDKFramework/MoPub.h>
#else
#import "MPViewabilityAdapter.h"
#endif

__attribute__((weak_import))
@interface MPViewabilityAdapterAvid : NSObject <
    MPViewabilityAdapterForWebView,
    MPViewabilityAdapterForNativeVideoView
>
@end
