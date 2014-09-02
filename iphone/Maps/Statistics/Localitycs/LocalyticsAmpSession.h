//
//  LocalyticsAmpSession.h
//  Copyright (C) 2013 Char Software Inc., DBA Localytics
//
//  This code is provided under the Localytics Modified BSD License.
//  A copy of this license has been distributed in a file called LICENSE
//  with this source code.
//
// Please visit www.localytics.com for more information.

#import "LocalyticsSession.h"

@protocol LocalyticsAmpSessionDelegate;

@interface LocalyticsAmpSession : LocalyticsSession

/*!
@property displayButtonOnLeft
@abstract Determines whether the dismiss button is shown on the left corner instead of the right
 Default value is true.
*/
@property (assign) BOOL displayButtonOnLeft;

/*!
 @property testModeEnabled
 @abstract Determines whether SDK is in test mode
 */
@property (assign) BOOL testModeEnabled;

/*!
 @property delegate
 @abstract Reference to an object that conforms to the LocalyticsSessionDelegate protocol.
 */
@property (nonatomic, assign) id<LocalyticsAmpSessionDelegate>ampDelegate;

/*!
 @property dismissButtonImage
 @abstract Image to use for the dismiss button (instead of the default).
 */
@property (nonatomic, readonly) UIImage* dismissButtonImage;

#pragma mark Public Methods
/*!
 @method shared
 @abstract Accesses the Session object.  This is a Singleton class which maintains
 a single session throughout your application.  It is possible to manage your own
 session, but this is the easiest way to access the Localytics object throughout your code.
 The class is accessed within the code using the following syntax:
 [[LocalyticsSession shared] functionHere]
 This is a AMP specific function designed to return the localytics session object as an AMP
 session object, rather than as its super class, the base LocalyticsSession
 */
+ (LocalyticsAmpSession *)shared;
- (BOOL)handleURL:(NSURL *)url;
- (void)ampTrigger:(NSString *)event;
- (void)ampTrigger:(NSString *)event attributes:(NSDictionary *)attributes;
- (void)ampTrigger:(NSString *)event attributes:(NSDictionary *)attributes reportAttributes:(NSDictionary *)reportAttributes;

- (void)setDismissButtonImageWithName:(NSString *)imageName;
- (void)setDismissButtonImageWithImage:(UIImage *)image;
@end

@protocol LocalyticsAmpSessionDelegate <NSObject>
@optional

- (void)localyticsWillDisplayAMPMessage;
- (void)localyticsDidDisplayAMPMessage;
- (void)localyticsWillHideAMPMessage;
- (void)localyticsDidHideAMPMessage;

@end
