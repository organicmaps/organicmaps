//
//  MPHTMLInterstitialCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPInterstitialCustomEvent.h"
#import "MPPrivateInterstitialCustomEventDelegate.h"

@interface MPHTMLInterstitialCustomEvent : MPInterstitialCustomEvent

@property (nonatomic, weak) id<MPPrivateInterstitialCustomEventDelegate> delegate;

@end
