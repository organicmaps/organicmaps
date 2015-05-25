//
//  PFSession.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>
#else
#import <ParseOSX/PFObject.h>
#import <ParseOSX/PFSubclassing.h>
#endif

PF_ASSUME_NONNULL_BEGIN

@class PFSession;

typedef void(^PFSessionResultBlock)(PFSession *PF_NULLABLE_S session, NSError *PF_NULLABLE_S error);

/*!
 `PFSession` is a local representation of a session.
 This class is a subclass of a <PFObject>,
 and retains the same functionality as any other subclass of <PFObject>.
 */
@interface PFSession : PFObject<PFSubclassing>

/*!
 @abstract The session token string for this session.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy, readonly) NSString *sessionToken;

/*!
 *Asynchronously* fetches a `PFSession` object related to the current user.

 @returns A task that is `completed` with an instance of `PFSession` class or is `faulted` if the operation fails.
 */
+ (BFTask *)getCurrentSessionInBackground;

/*!
 *Asynchronously* fetches a `PFSession` object related to the current user.

 @param block The block to execute when the operation completes.
 It should have the following argument signature: `^(PFSession *session, NSError *error)`.
 */
+ (void)getCurrentSessionInBackgroundWithBlock:(PF_NULLABLE PFSessionResultBlock)block;

@end

PF_ASSUME_NONNULL_END
