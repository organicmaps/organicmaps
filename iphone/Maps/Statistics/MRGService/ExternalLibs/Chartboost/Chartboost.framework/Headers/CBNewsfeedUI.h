/*
 * CBNewsfeedUI.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

/*! @abstract CBStory forward class declaration. */
@class CBStory;

/*!
 @typedef NS_ENUM(NSUInteger, CBNewsfeedAnimationType)
 
 @abstract
 Use to control how the CBNewsfeedUI should animate its display or dismiss animation.
 */
typedef NS_ENUM(NSUInteger, CBNewsfeedAnimationType) {
    /*! Slide up from screen bottom. */
    CBNewsfeedAnimationTypeSlideFromBottom,
    /*! Slide down from screen top. */
    CBNewsfeedAnimationTypeSlideFromTop,
    /*! Slide out from screen left. */
    CBNewsfeedAnimationTypeSlideFromLeft,
    /*! Slide out from screen right. */
    CBNewsfeedAnimationTypeSlideFromRight,
    /*! No animation, just appear. */
    CBNewsfeedAnimationTypeNone
};

/*!
 @typedef NS_ENUM(NSUInteger, CBNewsfeedUIPosition)
 
 @abstract
 Use to control where the CBNewsfeedUI should position itself on the screen.
 */
typedef NS_ENUM(NSUInteger, CBNewsfeedUIPosition) {
    /*! Horizonatal and vertical center. */
    CBNewsfeedUIPositionCenter,
    /*! Anchor to top of screen. */
    CBNewsfeedUIPositionTop,
    /*! Anchor to bottom of screen. */
    CBNewsfeedUIPositionBottom,
    /*! Anchor to left of screen. */
    CBNewsfeedUIPositionLeft,
    /*! Anchor to right of screen. */
    CBNewsfeedUIPositionRight
};

/*!
 @typedef NS_ENUM(NSUInteger, CBNotificationUIClickAction)
 
 @abstract
 Use to control how the Notification UI should respond when tapped.
 */
typedef NS_ENUM(NSUInteger, CBNotificationUIClickAction) {
    /*! Display the Newsfeed UI. */
    CBNotificationUIClickActionDisplayNewsfeedUI,
    /*! Perform link action. */
    CBNotificationUIClickActionPerformLink,
    /*! No action; just dismiss UI. */
    CBNotificationUIClickActionNone
};

/*!
 @protocol CBNewsfeedUIProtocol
 
 @abstract
 Provide an interface for implementing a custom Newsfeed UI to use with the Chartboost SDK.
 Implement this protocol in a class and assign it with [CBNewsfeed setUIClass:(Class)uiClass] to 
 override the default Newsfeed UI.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 
 All of the delegate methods are required.
 */
@protocol CBNewsfeedUIProtocol

@required

/*!
 @abstract
 Display the Newsfeed UI.
 
 @discussion Implement your custom logic to display the Newsfeed UI.
 */
+ (void)displayNewsfeed;

/*!
 @abstract
 Dismiss the Newsfeed UI.
 
 @discussion Implement your custom logic to dismiss the Newsfeed UI.
 */
+ (void)dismissNewsfeed;

/*!
 @abstract
 Check if the Newsfeed UI is visible.
 
 @return YES if the UI is visible, NO otherwise.
 
 @discussion Implement your custom logic to check if the Newsfeed UI is visible.
 */
+ (BOOL)isNewsfeedUIVisible;

/*!
 @abstract
 Display the Notification UI.
 
 @discussion Implement your custom logic to display the Notification UI.
 */
+ (void)displayNotification;

/*!
 @abstract
 Display the Notification UI for a specific CBStory.
 
 @param story The CBStory to display.
 
 @discussion Implement your custom logic to display the Notification UI.
 */
+ (void)displayNotification:(CBStory *)story;

/*!
 @abstract
 Dismiss the Notification UI.
 
 @discussion Implement your custom logic to dismiss the Notification UI.
 */
+ (void)dismissNotification;

/*!
 @abstract
 Check if the Notification UI is visible.
 
 @return YES if the UI is visible, NO otherwise.
 
 @discussion Implement your custom logic to check if the Notification UI is visible.
 */
+ (BOOL)isNotificationUIVisible;

@end

/*!
 @class CBNewsfeedUI
 
 @abstract
 Default Newsfeed UI provided by the Chartboost SDK.
 
 Implements the CBNewsfeedUIProtocol.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBNewsfeedUI : NSObject

/*!
 @abstract
 Get CGSize for Default NewsfeedUI.
 
 @return The CGSize of the parent frame for the default Newsfeed UI.
 
 @discussion You can use this to help size a custom header.
 */
+ (CGSize)getNewsfeedUISize;

#pragma mark - Setters

/*!
 @abstract
 Set anchor point for Newsfeed UI in portrait mode.
 
 @param position CBNewsfeedUIPosition
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedUIPositionBottom.
 */
+ (void)setNewsfeedUIPortraitPosition:(CBNewsfeedUIPosition)position;

/*!
 @abstract
 Set anchor point for Newsfeed UI in landscape mode.
 
 @param position CBNewsfeedUIPosition
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedUIPositionLeft.
 */
+ (void)setNewsfeedUILandscapePosition:(CBNewsfeedUIPosition)position;

/*!
 @abstract
 Set display animation for Newsfeed UI in portrait mode.
 
 @param animation CBNewsfeedAnimationType
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedAnimationTypeSlideFromBottom.
 */
+ (void)setPortraitDisplayAnimation:(CBNewsfeedAnimationType)animation;

/*!
 @abstract
 Set dismiss animation for Newsfeed UI in portrait mode.
 
 @param animation CBNewsfeedAnimationType
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedAnimationTypeSlideFromBottom.
 */
+ (void)setPortraitDismissAnimation:(CBNewsfeedAnimationType)animation;

/*!
 @abstract
 Set display animation for Newsfeed UI in lanscape mode.
 
 @param animation CBNewsfeedAnimationType
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedAnimationTypeSlideFromLeft.
 */
+ (void)setLandscapeDisplayAnimation:(CBNewsfeedAnimationType)animation;

/*!
 @abstract
 Set dismiss animation for Newsfeed UI in landscape mode.
 
 @param animation CBNewsfeedAnimationType
 
 @discussion Valid values located above in this file.
 
 Default is CBNewsfeedAnimationTypeSlideFromLeft.
 */
+ (void)setLandscapeDismissAnimation:(CBNewsfeedAnimationType)animation;

/*!
 @abstract
 Controls what the Notification UI should attempt to do on click.
 
 @param action CBNotificationUIClickAction
 
 @discussion Valid values located above in this file.
 
 Default is CBNotificationUIClickActionDisplayNewsfeedUI.
 */
+ (void)setNotificationUIClickAction:(CBNotificationUIClickAction)action;

/*!
 @abstract
 Set Newsfeed UI background color.
 
 @param color UIColor
 
 @discussion Set Newsfeed UI background color.
 */
+ (void)setNewsfeedBackgroundColor:(UIColor *)color;

/*!
 @abstract
 Set Newsfeed UI message cell background color.
 
 @param color UIColor
 
 @discussion Set Newsfeed UI message cell background color.
 */
+ (void)setNewsfeedMessageCellBackgroundColor:(UIColor *)color;

/*!
 @abstract
 Set Newsfeed UI header cell background color.
 
 @param color UIColor
 
 @discussion Set Newsfeed UI header cell background color.
 */
+ (void)setNewsfeedHeaderCellBackgroundColor:(UIColor *)color;

/*!
 @abstract
 Set Newsfeed UI message cell text color.
 
 @param color UIColor
 
 @discussion Set Newsfeed UI message cell text color.
 */
+ (void)setNewsfeedMessageCellTextColor:(UIColor *)color;

/*!
 @abstract
 Set Newsfeed UI header cell background color.
 
 @param color UIColor
 
 @discussion Set Newsfeed UI header cell background color.
 */
+ (void)setNewsfeedHeaderCellTextColor:(UIColor *)color;

/*!
 @abstract
 Set Newsfeed UI header cell text.
 
 @param text NSString
 
 @discussion Set Newsfeed UI header cell text.
 */
+ (void)setNewsfeedHeaderCellText:(NSString *)text;

/*!
 @abstract
 Set Notification UI text color.
 
 @param color UIColor
 
 @discussion Set Notification UI text color.
 */
+ (void)setNotificationTextColor:(UIColor *)color;

/*!
 @abstract
 Set Notification UI background color.
 
 @param color UIColor
 
 @discussion Set Notification UI background color.
 */
+ (void)setNotificationBackgroundColor:(UIColor *)color;

/*!
 @abstract
 Set Notification UI text font.
 
 @param font UIFont
 
 @discussion Set Notification UI text font.
 */
+ (void)setNotificationTextFont:(UIFont *)font;

/*!
 @abstract
 Set a custom header UIView for the Newsfeed UI.
 
 @param view UIView
 
 @discussion Set a custom header UIView for the Newsfeed UI.
 */
+ (void)setCustomNewsfeedHeaderView:(UIView *)view;

#pragma mark - Getters

/*!
 @abstract
 Get Newsfeed UI background color.
 
 @return UIColor
 
 @discussion Get Newsfeed UI background color.
 */
+ (UIColor *)getNewsfeedBackgroundColor;

/*!
 @abstract
 Get Newsfeed UI message cell background color.
 
 @return UIColor
 
 @discussion Get Newsfeed UI message cell background color.
 */
+ (UIColor *)getNewsfeedMessageCellBackgroundColor;

/*!
 @abstract
 Get Newsfeed UI header cell background color.
 
 @return UIColor
 
 @discussion Get Newsfeed UI header cell background color.
 */
+ (UIColor *)getNewsfeedHeaderCellBackgroundColor;

/*!
 @abstract
 Get Newsfeed UI message cell text color.
 
 @return UIColor
 
 @discussion Get Newsfeed UI message cell text color.
 */
+ (UIColor *)getNewsfeedMessageCellTextColor;

/*!
 @abstract
 Get Newsfeed UI header cell text color.
 
 @return UIColor
 
 @discussion Get Newsfeed UI header cell text color.
 */
+ (UIColor *)getNewsfeedHeaderCellTextColor;

/*!
 @abstract
 Get Newsfeed UI header cell text.
 
 @return NSString
 
 @discussion Get Newsfeed UI header cell text.
 */
+ (NSString *)getNewsfeedHeaderCellText;

/*!
 @abstract
 Get Notification UI text color.
 
 @return UIColor
 
 @discussion Get Notification UI text color.
 */
+ (UIColor *)getNotificationTextColor;

/*!
 @abstract
 Get Notification UI background color.
 
 @return UIColor
 
 @discussion Get Notification UI background color.
 */
+ (UIColor *)getNotificationBackgroundColor;

/*!
 @abstract
 Get Notification UI text font.
 
 @return UIFont
 
 @discussion Get Notification UI text font.
 */
+ (UIFont *)getNotificationTextFont;

/*!
 @abstract
 Get a custom header UIView for the Newsfeed UI.
 
 @return UIView
 
 @discussion Get a custom header UIView for the Newsfeed UI.
 */
+ (UIView *)getCustomNewsfeedHeaderView;

/*!
 @abstract
 Get the orientation of the CBNewsfeedUI.
 
 @return UIInterfaceOrientation
 
 @discussion Get the orientation of the CBNewsfeedUI.
 */
+ (UIInterfaceOrientation)getOrientation;

@end

