//
//  MPMediationSdkInitializable.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 Indicates that the implementer is initializable by the MoPub SDK when
 @c initializeSdkWithConfiguration:complete: is called, or whenever the
 mediated network needs to be initialized.
 */
@protocol MPMediationSdkInitializable <NSObject>

/**
 Called when the MoPub SDK requires the underlying mediation SDK to be initialized.

 @param parameters A dictionary containing any mediation SDK-specific information
 needed for initialization, such as app IDs and placement IDs.
 */
- (void)initializeSdkWithParameters:(NSDictionary * _Nullable)parameters;

@end
