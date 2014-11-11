/*!
 @header    GAI.h
 @abstract  Google Analytics iOS SDK Header
 @version   3.0
 @copyright Copyright 2013 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>
#import "GAILogger.h"
#import "GAITracker.h"
#import "GAITrackedViewController.h"


typedef NS_ENUM(NSUInteger, GAIDispatchResult) {
  kGAIDispatchNoData,
  kGAIDispatchGood,
  kGAIDispatchError
};

/*! Google Analytics product string.  */
extern NSString *const kGAIProduct;

/*! Google Analytics version string.  */
extern NSString *const kGAIVersion;

/*!
 NSError objects returned by the Google Analytics SDK may have this error domain
 to indicate that the error originated in the Google Analytics SDK.
 */
extern NSString *const kGAIErrorDomain;

/*! Google Analytics error codes.  */
typedef enum {
  // This error code indicates that there was no error. Never used.
  kGAINoError = 0,

  // This error code indicates that there was a database-related error.
  kGAIDatabaseError,

  // This error code indicates that there was a network-related error.
  kGAINetworkError,
} GAIErrorCode;

/*!
 Google Analytics iOS top-level class. Provides facilities to create trackers
 and set behaviorial flags.
 */
@interface GAI : NSObject

/*!
 For convenience, this class exposes a default tracker instance.
 This is initialized to `nil` and will be set to the first tracker that is
 instantiated in trackerWithTrackingId:. It may be overridden as desired.

 The GAITrackedViewController class will, by default, use this tracker instance.
 */
@property(nonatomic, assign) id<GAITracker> defaultTracker;

/*!
 The GAILogger to use.
 */
@property(nonatomic, retain) id<GAILogger> logger;

/*!
 When this is true, no tracking information will be gathered; tracking calls
 will effectively become no-ops. When set to true, all tracking information that
 has not yet been submitted. The value of this flag will be persisted
 automatically by the SDK.  Developers can optionally use this flag to implement
 an opt-out setting in the app to allows users to opt out of Google Analytics
 tracking.

 This is set to `NO` the first time the Google Analytics SDK is used on a
 device, and is persisted thereafter.
 */
@property(nonatomic, assign) BOOL optOut;

/*!
 If this value is positive, tracking information will be automatically
 dispatched every dispatchInterval seconds. Otherwise, tracking information must
 be sent manually by calling dispatch.

 By default, this is set to `120`, which indicates tracking information should
 be dispatched automatically every 120 seconds.
 */
@property(nonatomic, assign) NSTimeInterval dispatchInterval;

/*!
 When set to true, the SDK will record the currently registered uncaught
 exception handler, and then register an uncaught exception handler which tracks
 the exceptions that occurred using defaultTracker. If defaultTracker is not
 `nil`, this function will track the exception on the tracker and attempt to
 dispatch any outstanding tracking information for 5 seconds. It will then call
 the previously registered exception handler, if any. When set back to false,
 the previously registered uncaught exception handler will be restored.
 */
@property(nonatomic, assign) BOOL trackUncaughtExceptions;

/*!
 When this is 'YES', no tracking information will be sent. Defaults to 'NO'.
 */
@property(nonatomic, assign) BOOL dryRun;

/*! Get the shared instance of the Google Analytics for iOS class. */
+ (GAI *)sharedInstance;

/*!
 Creates or retrieves a GAITracker implementation with the specified name and
 tracking ID. If the tracker for the specified name does not already exist, then
 it will be created and returned; otherwise, the existing tracker will be
 returned. If the existing tracker for the respective name has a different
 tracking ID, that tracking ID is not changed by this method. If defaultTracker
 is not set, it will be set to the tracker instance returned here.

 @param name The name of this tracker. Must not be `nil` or empty.

 @param trackingID The tracking ID to use for this tracker.  It should be of
 the form `UA-xxxxx-y`.

 @return A GAITracker associated with the specified name. The tracker
 can be used to send tracking data to Google Analytics. The first time this
 method is called with a particular name, the tracker for that name will be
 returned, and subsequent calls with the same name will return the same
 instance. It is not necessary to retain the tracker because the tracker will be
 retained internally by the library.

 If an error occurs or the name is not valid, this method will return
 `nil`.
 */
- (id<GAITracker>)trackerWithName:(NSString *)name
                       trackingId:(NSString *)trackingId;

/*!
 Creates or retrieves a GAITracker implementation with name equal to
 the specified tracking ID. If the tracker for the respective name does not
 already exist, it is created, has it's tracking ID set to |trackingId|,
 and is returned; otherwise, the existing tracker is returned. If the existing
 tracker for the respective name has a different tracking ID, that tracking ID
 is not changed by this method. If defaultTracker is not set, it is set to the
 tracker instance returned here.

 @param trackingID The tracking ID to use for this tracker.  It should be of
 the form `UA-xxxxx-y`. The name of the tracker will be the same as trackingID.

 @return A GAITracker associated with the specified trackingID. The tracker
 can be used to send tracking data to Google Analytics. The first time this
 method is called with a particular trackingID, the tracker for the respective
 name will be returned, and subsequent calls with the same trackingID
 will return the same instance. It is not necessary to retain the tracker
 because the tracker will be retained internally by the library.

 If an error occurs or the trackingId is not valid, this method will return
 `nil`.
 */
- (id<GAITracker>)trackerWithTrackingId:(NSString *)trackingId;

/*!
 Remove a tracker from the trackers dictionary. If it is the default tracker,
 clears the default tracker as well.

 @param name The name of the tracker.
 */
- (void)removeTrackerByName:(NSString *)name;

/*!
 Dispatches any pending tracking information.

 Note that this does not have any effect on dispatchInterval, and can be used in
 conjunction with periodic dispatch. */
- (void)dispatch;

/*!
 Dispatches the next tracking beacon in the queue, calling completionHandler when
 the tracking beacon has either been sent (returning kGAIDispatchGood) or an error has resulted
 (returning kGAIDispatchError).  If there is no network connection or there is no data to send,
 kGAIDispatchNoData is returned.

 Calling this method with a nil completionHandler is the same as calling the dispatch
 above.

 This method can be used for background data fetching in iOS 7.0 or later.

 It would be wise to call this when application is exiting to initiate the
 submission of any unsubmitted tracking information. Note that this does not
 have any effect on dispatchInterval, and can be used in conjunction with
 periodic dispatch. */
- (void)dispatchWithCompletionHandler:(void (^)(GAIDispatchResult))completionHandler;
@end
