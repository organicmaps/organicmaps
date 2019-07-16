//
//  PWRichMediaManager.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2018
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE

#import "PWRichMediaStyle.h"

#endif

typedef NS_ENUM(NSUInteger, PWRichMediaSource) {
    /**
     RichMedia is presented from push notification.
     */
    PWRichMediaSourcePush,
    
    /**
     RichMedia is presented from In-App.
     */
    PWRichMediaSourceInApp
};


/*
 `PWRichMedia` class represents Rich Media page object.
 */
@interface PWRichMedia: NSObject

/**
 Rich Media presenter type.
 */
@property (nonatomic, readonly) PWRichMediaSource source;

/**
 Content of the Rich Media. For PWRichMediaSourceInApp it's equal to In-App code, for PWRichMediaSourcePush it's equal to Rich Media code.
 */
@property (nonatomic, readonly) NSString *content;

/**
 Checks if PWRichMediaSourceInApp is a required In-App. Always returns YES for PWRichMediaSourcePush.
 */
@property (nonatomic, readonly, getter=isRequired) BOOL required;

@end


@class PWRichMediaManager;

/**
 Interface for Rich Media presentation managing.
 */
@protocol PWRichMediaPresentingDelegate <NSObject>

@optional

/**
 Checks the delegate whether the Rich Media should be displayed.
 */
- (BOOL)richMediaManager:(PWRichMediaManager *)richMediaManager shouldPresentRichMedia:(PWRichMedia *)richMedia;

/**
 Tells the delegate that Rich Media has been displayed.
 */
- (void)richMediaManager:(PWRichMediaManager *)richMediaManager didPresentRichMedia:(PWRichMedia *)richMedia;

/**
 Tells the delegate that Rich Media has been closed.
 */
- (void)richMediaManager:(PWRichMediaManager *)richMediaManager didCloseRichMedia:(PWRichMedia *)richMedia;

/**
 Tells the delegate that error during Rich Media presenting has been occured.
 */
- (void)richMediaManager:(PWRichMediaManager *)richMediaManager presentingDidFailForRichMedia:(PWRichMedia *)richMedia withError:(NSError *)error;

@end


/*
 `PWRichMediaManager` class offers access to the singleton-instance of the manager responsible for Rich Media presentation.
 */
@interface PWRichMediaManager : NSObject

#if TARGET_OS_IPHONE

/**
 Style for Rich Media presenting.
 */
@property (nonatomic) PWRichMediaStyle *richMediaStyle;

#endif

/**
 Delegate for Rich Media presentation managing.
 */
@property (nonatomic) id<PWRichMediaPresentingDelegate> delegate;

/**
 A singleton object that represents the rich media manager.
 */
+ (instancetype)sharedManager;

/**
 Presents the rich media object.
 */
- (void)presentRichMedia:(PWRichMedia *)richMedia;

@end

