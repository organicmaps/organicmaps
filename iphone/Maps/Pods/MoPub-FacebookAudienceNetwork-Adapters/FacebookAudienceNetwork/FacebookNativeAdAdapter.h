//
//  FacebookNativeAdAdapter.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#if __has_include(<MoPub/MoPub.h>)
    #import <MoPub/MoPub.h>
#elif __has_include(<MoPubSDKFramework/MoPub.h>)
    #import <MoPubSDKFramework/MoPub.h>
#else
    #import "MPNativeAdAdapter.h"
#endif

@class FBNativeAdBase;

@interface FacebookNativeAdAdapter : NSObject <MPNativeAdAdapter>

@property (nonatomic, weak) id<MPNativeAdAdapterDelegate> delegate;
@property (nonatomic, readonly) FBNativeAdBase *fbNativeAdBase;

- (instancetype)initWithFBNativeAdBase:(FBNativeAdBase *)fbNativeAdBase adProperties:(NSDictionary *)adProps;

@end
