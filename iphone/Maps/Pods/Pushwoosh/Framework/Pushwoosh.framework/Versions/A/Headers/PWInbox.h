//
//  PWInbox.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2017
//

#import <Foundation/Foundation.h>


/**
 The notification arriving on the Inbox messages renewal 
 */
FOUNDATION_EXPORT NSString * const PWInboxMessagesDidUpdateNotification;

/**
 The notification arriving when a push message is added to Inbox
 */
FOUNDATION_EXPORT NSString * const PWInboxMessagesDidReceiveInPushNotification;

/**
 The Inbox message type. Plain = without any action, Richmedia = contains a Rich media page, URL = contains remote URL, Deeplink = contains Deeplink
 */
typedef NS_ENUM(NSInteger, PWInboxMessageType) {
    PWInboxMessageTypePlain = 0,
    PWInboxMessageTypeRichmedia = 1,
    PWInboxMessageTypeURL = 2,
    PWInboxMessageTypeDeeplink = 3
};


/**
 `PWInboxMessageProtocol` The protocol describing the Inbox message.
 */
@protocol PWInboxMessageProtocol <NSObject>

@required

@property (readonly, nonatomic) NSString *code;
@property (readonly, nonatomic) NSString *title;
@property (readonly, nonatomic) NSString *imageUrl;
@property (readonly, nonatomic) NSString *message;
@property (readonly, nonatomic) NSDate *sendDate;
@property (readonly, nonatomic) PWInboxMessageType type;
//! Inbox Message which is read, see + (void)readMessagesWithCodes:(NSArray<NSString *> *)codes
@property (readonly, nonatomic) BOOL isRead;
//! Action of the Inbox Message is performed (if true, an action was performed in the Inbox see + (void)performActionForMessageWithCode:(NSString *)code or an action was performed on the push tap )
@property (readonly, nonatomic) BOOL isActionPerformed;
@property (readonly, nonatomic) NSDictionary *actionParams;
@property (readonly, nonatomic) NSString *attachmentUrl;

@end

@interface PWInbox : NSObject

- (instancetype)init NS_UNAVAILABLE;

/**
 Get the number of the PWInboxMessageProtocol with no action performed
 
 @param completion - if successful, return the number of the InboxMessages with no action performed. Otherwise, return error
 */
+ (void)messagesWithNoActionPerformedCountWithCompletion:(void (^)(NSInteger count, NSError *error))completion;

/**
 Get the number of the unread PWInboxMessageProtocol
 
 @param completion - if successful, return the number of the unread InboxMessages. Otherwise, return error
 */
+ (void)unreadMessagesCountWithCompletion:(void (^)(NSInteger count, NSError *error))completion;

/**
 Get the total number of the PWInboxMessageProtocol
 
 @param completion - if successful, return the total number of the InboxMessages. Otherwise, return error
 */
+ (void)messagesCountWithCompletion:(void (^)(NSInteger count, NSError *error))completion;

/**
 Get the collection of the PWInboxMessageProtocol that the user received
 
 @param completion - if successful, return the collection of the InboxMessages. Otherwise, return error
 */
+ (void)loadMessagesWithCompletion:(void (^)(NSArray<NSObject<PWInboxMessageProtocol> *> *messages, NSError *error))completion;

/**
 Call this method to mark the list of InboxMessageProtocol as read
 
 @param codes of the inboxMessages
 */
+ (void)readMessagesWithCodes:(NSArray<NSString *> *)codes;

/**
 Call this method, when the user clicks on the InboxMessageProtocol and the message’s action is performed
 
 @param code of the inboxMessage that the user tapped
 */
+ (void)performActionForMessageWithCode:(NSString *)code;

/**
 Call this method, when the user deletes the list of InboxMessageProtocol manually
 
 @param codes of the list of InboxMessageProtocol.code that the user deleted
 */
+ (void)deleteMessagesWithCodes:(NSArray<NSString *> *)codes;

/**
 Subscribe for messages arriving with push notifications. @warning You need to unsubscribe by calling the removeObserver method, if you don't want to receive notifications
 
 @param completion - return the collection of the InboxMessages.
 */
+ (id<NSObject>)addObserverForDidReceiveInPushNotificationCompletion:(void (^)(NSArray<NSObject<PWInboxMessageProtocol> *> *messagesAdded))completion;

/**
 Subscribe for messages arriving when a message is deleted, added, or updated. @warning You need to unsubscribe by calling the removeObserver method, if you don't want to receive notifications
 
 @param completion - return the collection of the InboxMessages.
 */
+ (id<NSObject>)addObserverForUpdateInboxMessagesCompletion:(void (^)(NSArray<NSString *> *messagesDeleted,
                                                                      NSArray<NSObject<PWInboxMessageProtocol> *> *messagesAdded,
                                                                      NSArray<NSObject<PWInboxMessageProtocol> *> *messagesUpdated))completion;

/**
 Unsubscribes from notifications
 
 @param observer - Unsubscribes observer
 */
+ (void)removeObserver:(id<NSObject>)observer;

@end
