//
//  MMInterstitial.h
//  MMSDK
//
//  Copyright (c) 2013 Millennial Media Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MMSDK.h"

@interface MMInterstitial : NSObject

// Fetch an interstitial for a given APID
+ (void)fetchWithRequest:(MMRequest *)request
                    apid:(NSString *)apid
            onCompletion:(MMCompletionBlock)callback;

// Check if an ad is available for a given APID
+ (BOOL)isAdAvailableForApid:(NSString *)apid;

// Display an interstitial for a given APID. ViewController is required. Orientation is optional, use 0 if no preference.
+ (void)displayForApid:(NSString *)apid
    fromViewController:(UIViewController *)viewController
       withOrientation:(UIInterfaceOrientation)overlayOrientation
          onCompletion:(MMCompletionBlock)callback;

@end
