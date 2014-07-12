//
//  GPPDeepLink.h
//  Google+ iOS SDK
//
//  Copyright 2012 Google Inc.
//
//  Use of this SDK is subject to the Google+ Platform Terms of Service:
//  https://developers.google.com/+/terms
//

#import <Foundation/Foundation.h>

@class GPPDeepLink;

// A protocol optionally implemented by the client of |GPPDeepLink|.
@protocol GPPDeepLinkDelegate

// Notifies the client that a deep link has been received either from
// |readDeepLinkAfterInstall| or |handleURL:sourceApplication:annotation:|.
- (void)didReceiveDeepLink:(GPPDeepLink *)deepLink;

@end

// This class handles a deep link within a share posted on Google+.
// For more information on deep links, see
// http://developers.google.com/+/mobile/ios/share .
@interface GPPDeepLink : NSObject

// Sets the delegate to handle the deep link.
+ (void)setDelegate:(id<GPPDeepLinkDelegate>)delegate;

// Returns a |GPPDeepLink| for your app to handle, or |nil| if not found. The
// deep-link ID can be obtained from |GPPDeepLink|. It is stored when a user
// clicks a link to your app from a Google+ post, but hasn't yet installed your
// app. The user will be redirected to the App Store to install your app. This
// method should be called on or near your app launch to take the user to
// deep-link ID within your app. The delegate will be called if set and if a
// deep link is found.
+ (GPPDeepLink *)readDeepLinkAfterInstall;

// This method should be called from your |UIApplicationDelegate|'s
// |application:openURL:sourceApplication:annotation|. Returns
// |GooglePlusDeepLink| if |GooglePlusDeepLink| handled this URL, |nil|
// otherwise. The delegate will be called if set and if a deep link is found.
// Also see |handleURL:sourceApplication:annotation:| in |GPPURLHandler|.
+ (GPPDeepLink *)handleURL:(NSURL *)url
         sourceApplication:(NSString *)sourceApplication
                annotation:(id)annotation;

// The deep-link ID in |GPPDeepLink| that was passed to the app.
- (NSString *)deepLinkID;

// This instance method indicates where the user came from before arriving in
// your app. This method is provided for you to collect engagement metrics.
// For the possible values, see
// http://developers.google.com/+/mobile/ios/source-values .
- (NSString *)source;

@end
