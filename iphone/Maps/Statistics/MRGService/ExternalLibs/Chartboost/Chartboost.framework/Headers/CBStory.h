/*
 * CBStory.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

/*!
 @class CBStory
 
 @abstract
 Provide methods to interact with individual Newsfeed messages and get data from them.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBStory : NSObject

/*! @abstract Unique identifier. */
@property (nonatomic, strong, readonly) NSString *storyID;
/*! @abstract Unix timestamp of message creation time. */
@property (nonatomic, assign, readonly) NSUInteger storySent;
/*! @abstract Message title. */
@property (nonatomic, strong, readonly) NSString *storyTitle;
/*! @abstract Message body. */
@property (nonatomic, strong, readonly) NSString *storyContent;
/*! @abstract URL for message icon. */
@property (nonatomic, strong, readonly) NSString *storyImageURL;
/*! @abstract Total views for this user for this message. */
@property (nonatomic, assign, readonly) NSUInteger storyViews;
/*! @abstract Total clicks for this user for this message. */
@property (nonatomic, assign, readonly) NSUInteger storyClicks;
/*! @abstract If message uses time based expiration this is the unix timestamp of that time. */
@property (nonatomic, assign, readonly) NSUInteger storyExpires;
/*! @abstract Max clicks allowed for this message before expiration. */
@property (nonatomic, assign, readonly) NSUInteger storyMaxClicks;
/*! @abstract Max views allowed for this message before expiration. */
@property (nonatomic, assign, readonly) NSUInteger storyMaxViews;
/*! @abstract The URL action to perform on clicking the message. */
@property (nonatomic, strong, readonly) NSString *storyLink;
/*! @abstract If this message should show a notification. */
@property (nonatomic, assign, readonly) BOOL storyShowNotification;
/*! @abstract If this message should show an expiration time. */
@property (nonatomic, assign, readonly) BOOL storyShowExpiration;
/*! @abstract When the message has been viewed. */
@property (nonatomic, assign, readonly) NSUInteger storyViewed;
/*! @abstract Total number of hours til expiration. */
@property (nonatomic, assign, readonly) NSUInteger storyExpiresHours;

/*!
 @abstract
 Mark a Newsfeed message as viewed from the Newsfeed UI.
 
 @param successBlock Function callback on successfully sending data to server.
 
 @param failureBlock Function callback on failing sending data to server.
 
 @discussion This method will first locally mark the CBStory as viewed and then attempt
 to notify the Chartboost API servers of the change.  If this request fails the CBStory
 will revert the local change.
 */
- (void)markViewedWithSuccessBlock:(void (^)(CBStory* story))successBlock
                  withFailureBlock:(void (^)(NSError *error, NSDictionary *response))failureBlock;

/*!
 @abstract
 Mark a Newsfeed message as clicked from the Newsfeed UI.
 
 @param successBlock Function callback on successfully sending data to server.
 
 @param failureBlock Function callback on failing sending data to server.
 
 @discussion This method will first locally mark the CBStory as clicked and then attempt
 to notify the Chartboost API servers of the change.  If this request fails the CBStory
 will revert the local change.
 */
- (void)markClickedWithSuccessBlock:(void (^)(CBStory* story))successBlock
                   withFailureBlock:(void (^)(NSError *error, NSDictionary *response))failureBlock;


/*!
 @abstract
 Mark a Newsfeed message as clicked from the Notification UI.
 
 @param successBlock Function callback on successfully sending data to server.
 
 @param failureBlock Function callback on failing sending data to server.
 
 @discussion This method will first locally mark the CBStory as clicked and then attempt
 to notify the Chartboost API servers of the change.  If this request fails the CBStory
 will revert the local change.
 */
- (void)markNotificationClickedWithSuccessBlock:(void (^)(CBStory* story))successBlock
                               withFailureBlock:(void (^)(NSError *error, NSDictionary *response))failureBlock;


/*!
 @abstract
 Mark a Newsfeed message as viewed from the Notification UI.
 
 @param successBlock Function callback on successfully sending data to server.
 
 @param failureBlock Function callback on failing sending data to server.
 
 @discussion This method will first locally mark the CBStory as viewed and then attempt
 to notify the Chartboost API servers of the change.  If this request fails the CBStory
 will revert the local change.
 */
- (void)markNotificationViewedWithSuccessBlock:(void (^)(CBStory* story))successBlock
                              withFailureBlock:(void (^)(NSError *error, NSDictionary *response))failureBlock;

@end
