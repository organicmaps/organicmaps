//
//  GPPShare.h
//  Google+ iOS SDK
//
//  Copyright 2012 Google Inc.
//
//  Use of this SDK is subject to the Google+ Platform Terms of Service:
//  https://developers.google.com/+/terms
//

// To allow a user to share with Google+, please follow these steps:
//
// 0. Create a project on Google API console,
//    https://code.google.com/apis/console . Under "API Access", create a
//    client ID as "Installed application" with the type "iOS", and
//    register the bundle ID of your app.
//
// 1. Initialize the |GPPSignIn| instance with your registered client ID,
//    and get the |GPPShare| instance.
//
//    [[GPPSignIn shareInstance] setClientID:myClientID];
//    GPPShare *gppShare = [GPPShare sharedInstance];
//
// 2. In the code where the share dialog will be opened,
//
//    [[gppShare shareDialog] open];
//
//    you can optionally call any of the |GPPShareBuilder| methods before
//    calling |open|, for example, if there is a particular URL resource to be
//    shared, or if you want to set text to prefill user comment in the share
//    dialog, such as:
//
//    NSURL *urlToShare = [NSURL URLWithString:@"http://www.google.com/"];
//    NSString *prefillText = @"You probably already know this site...";
//    [[[[gppShare shareDialog] setURLToShare:urlToShare]
//        setPrefillText:prefillText] open];
//
// 3. In the '<YourApp>-info.plist' settings for your app, add a URL type to be
//    handled by your app. Make the URL scheme the same as your app bundle ID.
//
// 4. In your application delegate, implement:
//    - (BOOL)application:(NSString*)application
//                openURL:(NSURL *)url
//      sourceApplication:(NSString*)sourceApplication
//             annotation:(id)annotation {
//      if ([gppShare handleURL:url
//            sourceApplication:sourceApplication
//                   annotation:annotation]) {
//        return YES;
//      }
//      // Other handling code here...
//    }
//
// 5. Optionally, if you want to be notified of the result of the share action,
//    have a delegate class implement |GPPShareDelegate|, for example:
//
//    @interface MyDelegateClass : NSObject<GPPShareDelegate>;
//
//    - (void)finishedSharing:(BOOL)shared {
//      // The share action was successful if |shared| is YES.
//    }
//
//    MyDelegateClass *myDelegate = [[MyDelegateClass alloc] init];
//    gppShare.delegate = myDelegate;

#import <Foundation/Foundation.h>

@class GPPSignIn;

// The protocol to receive the result of the share action.
@protocol GPPShareDelegate

// Reports the status of the share action, |shared| is |YES| if user has
// successfully shared her post, |NO| otherwise, such as if the user canceled
// the post.
- (void)finishedSharing:(BOOL)shared;

@end

// The builder protocol to open the share dialog.
// For more information on sharing, see
// http://developers.google.com/+/mobile/ios/share .
@protocol GPPShareBuilder<NSCopying>

// Sets the URL resource to be shared.
- (id<GPPShareBuilder>)setURLToShare:(NSURL *)urlToShare;

// Sets the text to prefill user's comment in the share dialog.
- (id<GPPShareBuilder>)setPrefillText:(NSString *)prefillText;

// Sets the title, description, and thumbnail URL of the shared content preview
// in the share dialog. Only set these fields if you are sharing with a content
// deep link and don't have a URL resource. |title| is required.
- (id<GPPShareBuilder>)setTitle:(NSString *)title
                    description:(NSString *)description
                   thumbnailURL:(NSURL *)thumbnailURL;

// Sets the content deep-link ID that takes the user straight to your shared
// content. Only set this field if you want the content deep-linking feature.
// The content deep-link ID can either be a fully qualified URI, or URI path,
// which can be up to 512 characters in length.
- (id<GPPShareBuilder>)setContentDeepLinkID:(NSString *)contentDeepLinkID;

// Sets the call-to-action button of the shared content preview.
// The call-to-action button consists of a label, URL, and deep-link ID.
// The |label| is a string key defined under "data-calltoactionlabel" on
// http://developers.google.com/+/web/share/interactive#button_attr_calltoactionlabel
// that maps to the actual button text.
// You must set either the |url| or |deepLinkID|, or both.
// The |url| is where the user is taken to after tapping on the button.
// The |deepLinkID| is the call-to-action deep-link ID that takes the user
// straight to a specific action in your app. It can either be a fully qualified
// URI, or URI path, which can be up to 512 characters in length.
// Note: In order to set the call-to-action button:
// 1. User must have been authenticated with scopes including
//    "https://www.googleapis.com/auth/plus.login".
// 2. Either |setURLToShare:| or |setTitle:description:thumbnailURL:| must also
//    be called.
- (id<GPPShareBuilder>)setCallToActionButtonWithLabel:(NSString *)label
                                                  URL:(NSURL *)url
                                           deepLinkID:(NSString *)deepLinkID;

// Opens the share dialog. Returns |NO| if there was an error, |YES| otherwise.
- (BOOL)open;

@end

// The primary class for the share action on Google+.
// For more information on sharing, see
// http://developers.google.com/+/mobile/ios/share .
@interface GPPShare : NSObject

// The object to be notified when the share action has finished.
@property (nonatomic, assign) id<GPPShareDelegate> delegate;

// Returns a shared |GPPShare| instance.
// |[GPPSignIn sharedInstance].clientID| must be initialized with a client ID
// registered in the Google API console, https://code.google.com/apis/console/
// with the app's bundle ID.
+ (GPPShare *)sharedInstance;

// Returns a share dialog builder instance. Call its |open| method to
// create the dialog after setting the parameters as needed.
- (id<GPPShareBuilder>)shareDialog;

// This method should be called from your |UIApplicationDelegate|'s
// |application:openURL:sourceApplication:annotation|. Returns |YES| if
// |GPPShare| handled this URL.
// Also see |handleURL:sourceApplication:annotation:| in |GPPURLHandler|.
- (BOOL)handleURL:(NSURL *)url
    sourceApplication:(NSString *)sourceApplication
           annotation:(id)annotation;

@end
