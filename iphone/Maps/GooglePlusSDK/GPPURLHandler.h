//
//  GPPURLHandler.h
//  Google+ iOS SDK
//
//  Copyright 2013 Google Inc.
//
//  Use of this SDK is subject to the Google+ Platform Terms of Service:
//  https://developers.google.com/+/terms
//

#import <Foundation/Foundation.h>

@interface GPPURLHandler : NSObject

// Calls |handleURL:sourceApplication:annotation:| for
// |[GPPSignIn sharedInstance]|, |[GPPShare sharedInstance]|, and
// |GPPDeepLink|, and returns |YES| if any of them handles the URL.
// This method can be called from your |UIApplicationDelegate|'s
// |application:openURL:sourceApplication:annotation| instead of calling
// those methods individually.
+ (BOOL)handleURL:(NSURL *)url
    sourceApplication:(NSString *)sourceApplication
           annotation:(id)annotation;

@end
