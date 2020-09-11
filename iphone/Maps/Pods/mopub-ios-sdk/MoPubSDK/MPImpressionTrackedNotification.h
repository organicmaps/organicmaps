//
//  MPImpressionTrackedNotification.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

/**
 Notification fired when an impression is tracked. The adunit ID will always be
 included in the @c NSNotification.userData dictionary. If the server returned
 impression data, the @c MPImpressionData object will be included in the
 @c NSNotification.userData dictionary. The sender can be determined by
 querying @c NSNotification.object.
 */
extern NSString * const kMPImpressionTrackedNotification;

/**
 The @c MPImpressionData object for the given impression, or @c nil if the server did not send
 impression data with this impression.
 */
extern NSString * const kMPImpressionTrackedInfoImpressionDataKey;

/**
 The adunit ID associated with the impression.
 */
extern NSString * const kMPImpressionTrackedInfoAdUnitIDKey;
