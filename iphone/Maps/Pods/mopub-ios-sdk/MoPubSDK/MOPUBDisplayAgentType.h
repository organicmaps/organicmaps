//
//  MOPUBDisplayAgentType.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

typedef NS_ENUM(NSInteger, MOPUBDisplayAgentType) {
    /**
     Use in-app views for display agent without escaping the app. @c SFSafariViewController is used
     for web browsing, and @c SKStoreProductViewController is used for supported App Store links.
     */
    MOPUBDisplayAgentTypeInApp = 0,

    /**
     Use the iOS Native Safari browser app for display agent.
     */
    MOPUBDisplayAgentTypeNativeSafari,

    /**
     This exists for historical reason, and it behaves the same as @c MOPUBDisplayAgentTypeInApp.
     */
    MOPUBDisplayAgentTypeSafariViewController __attribute__((deprecated))
};
